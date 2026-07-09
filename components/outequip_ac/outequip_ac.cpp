#include "outequip_ac.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include <cmath>
#include <cstring>

namespace esphome {
namespace outequip_ac {

void OutEquipACSwitch::write_state(bool state) {
  if (parent_ != nullptr) {
    parent_->set_lcd_state(state);
  }
}

void OutEquipACSwitch::setup() {
  auto restored = this->get_initial_state_with_restore_mode();
  if (restored.has_value()) {
    if (restored.value()) {
      this->turn_on();
    } else {
      this->turn_off();
    }
  }
}

light::LightTraits OutEquipACLight::get_traits() {
  light::LightTraits traits{};
  traits.set_supported_color_modes({light::ColorMode::ON_OFF});
  return traits;
}

void OutEquipACLight::write_state(light::LightState *state) {
  if (parent_ != nullptr) {
    parent_->set_light_state(state->current_values.is_on());
  }
}

void OutEquipACNumber::control(float value) {
  if (parent_ != nullptr) {
    switch (type_) {
    case UNDERVOLT:
      parent_->set_undervolt_protect(value);
      break;
    case RAW_KEY:
      parent_->set_raw_command_key(value);
      break;
    case RAW_VALUE:
      parent_->set_raw_command_value(value);
      break;
    }
  }
}

void OutEquipACButton::press_action() {
  if (parent_ != nullptr) {
    switch (type_) {
    case RAW_SEND:
      parent_->send_raw_command_value();
      break;
    case RAW_QUERY:
      parent_->query_raw_command_key();
      break;
    }
  }
}

void OutEquipAC::set_lcd_state(bool state) {
  ESP_LOGD("outequip_ac", "Setting LCD state to %s", state ? "ON" : "OFF");
  EnqueueFrame(ACFramer::Key::LCD,
               state ? static_cast<uint16_t>(ACFramer::OnOffValue::On)
                     : static_cast<uint16_t>(ACFramer::OnOffValue::Off));
  if (lcd_switch_ != nullptr) {
    lcd_switch_->publish_state(state);
  }
}

void OutEquipAC::set_light_state(bool state) {
  ESP_LOGD("outequip_ac", "Setting Light state to %s", state ? "ON" : "OFF");
  EnqueueFrame(ACFramer::Key::Light,
               state ? static_cast<uint16_t>(ACFramer::LightValue::On)
                     : static_cast<uint16_t>(ACFramer::LightValue::Off));
}

void OutEquipAC::set_undervolt_protect(float voltage) {
  const float clamped = esphome::clamp(voltage, kUndervoltProtectMinVolts,
                                      kUndervoltProtectMaxVolts);
  const auto decivolts = static_cast<uint16_t>(std::round(clamped * 10.0f));
  ESP_LOGD("outequip_ac", "Setting undervolt protection to %.1f V", clamped);
  EnqueueFrame(ACFramer::Key::UndervoltProtect, decivolts);
  if (undervolt_number_ != nullptr) {
    undervolt_number_->publish_state(decivolts / 10.0f);
  }
}

void OutEquipAC::set_raw_command_key(float key) {
  raw_command_key_ =
      static_cast<uint8_t>(esphome::clamp(std::round(key), 1.0f, 255.0f));
  ESP_LOGD("outequip_ac", "Staged raw UART key %u", raw_command_key_);
}

void OutEquipAC::set_raw_command_value(float value) {
  raw_command_value_ =
      static_cast<uint16_t>(esphome::clamp(std::round(value), 0.0f, 65535.0f));
  ESP_LOGD("outequip_ac", "Staged raw UART value %u", raw_command_value_);
}

void OutEquipAC::send_raw_command_value() {
  ESP_LOGD("outequip_ac", "Sending raw UART key %u value %u", raw_command_key_,
           raw_command_value_);
  if (!EnqueueRawFrame(raw_command_key_, raw_command_value_)) {
    ESP_LOGW("outequip_ac", "Rejected raw UART key %u value %u", raw_command_key_,
             raw_command_value_);
  }
}

void OutEquipAC::query_raw_command_key() {
  ESP_LOGD("outequip_ac", "Querying raw UART key %u", raw_command_key_);
  if (!EnqueueRawFrame(raw_command_key_, ACFramer::kQueryVal)) {
    ESP_LOGW("outequip_ac", "Rejected raw UART query key %u", raw_command_key_);
  }
}

void OutEquipAC::setup() {
  this->set_supported_custom_fan_modes(
      {kFanMode1, kFanMode2, kFanMode3, kFanMode4, kFanMode5Auto});
  EnqueueFrame(ACFramer::Key::Active, 0);
  last_frame_sent = millis();
}

void OutEquipAC::loop() {
  if (millis() - last_frame_sent >= kFrameIntervalMs) {
    MaybeSendCurFrame();
  }

  auto publish_sensor = [](sensor::Sensor *sensor, float value) {
    if (sensor != nullptr && (!sensor->has_state() || sensor->state != value)) {
      ESP_LOGD("outequip_ac", "Publishing sensor state for '%s': %.1f",
               sensor->get_name().c_str(), value);
      sensor->publish_state(value);
    }
  };

  while (this->available()) {
    uint8_t c = this->read();
    if (!rxFramer.FrameData(c)) {
      if (rxFramer.buffer_pos() > 0) {
        num_frames_failed_++;
      } else {
        num_spurious_bytes_rx_++;
      }
      PublishCounterSensors();
      rxFramer.Reset();
    } else if (rxFramer.HasFullFrame()) {
      num_frames_rx_++;
      PublishCounterSensors();
      const auto key = rxFramer.GetKey();
      const auto value = rxFramer.GetValue();
      PublishRawSensor(key, value);

      bool climate_changed = false;

      switch (key) {
      case ACFramer::Key::Power: {
        const auto old_power_state = cur_power_state_;
        cur_power_state_ = static_cast<ACFramer::OnOffValue>(value);
        if (lcd_switch_ != nullptr) {
          if (cur_power_state_ == ACFramer::OnOffValue::Off) {
            lcd_switch_->publish_state(false);
            lcd_switch_->set_has_state(true);
          } else if (old_power_state == ACFramer::OnOffValue::Off &&
                     cur_power_state_ == ACFramer::OnOffValue::On) {
            lcd_switch_->publish_state(true);
            lcd_switch_->set_has_state(true);
          }
        }
        break;
      }
      case ACFramer::Key::Mode:
        cur_mode_ = static_cast<ACFramer::ModeValue>(value);
        break;
      case ACFramer::Key::SetTemperature: {
        // Some Summit2 boards report the UART setpoint in Celsius even when
        // the front-panel display is configured for Fahrenheit.
        float new_target = (value >= 17 && value <= 30)
                               ? value
                               : (value - 32.0f) * 5.0f / 9.0f;
        if (this->target_temperature != new_target) {
          ESP_LOGD("outequip_ac", "Climate target temp changed to %.1f C",
                   new_target);
          this->target_temperature = new_target;
          climate_changed = true;
        }
        break;
      }
      case ACFramer::Key::FanSpeed:
        cur_fan_speed_ = value;
        if (const char *new_fan_mode = FanValueToCustomFanMode(value)) {
          ESP_LOGD("outequip_ac", "Climate fan speed changed to %s",
                   new_fan_mode);
          if (this->set_custom_fan_mode_(new_fan_mode)) {
            climate_changed = true;
          }
        } else if (this->has_custom_fan_mode() || this->fan_mode.has_value()) {
          this->clear_custom_fan_mode_();
          this->fan_mode.reset();
          climate_changed = true;
        }
        break;
      case ACFramer::Key::UndervoltProtect:
        publish_sensor(undervolt_sensor_, value / 10.0f);
        if (undervolt_number_ != nullptr &&
            (std::isnan(undervolt_number_->state) ||
             undervolt_number_->state != value / 10.0f)) {
          undervolt_number_->publish_state(value / 10.0f);
        }
        break;
      case ACFramer::Key::OvervoltProtect:
        publish_sensor(overvolt_sensor_, value);
        break;
      case ACFramer::Key::IntakeAirTemp: {
        int8_t intake_temp = static_cast<int8_t>(value & 0xFF);
        publish_sensor(intake_temp_sensor_, intake_temp);
        if (this->current_temperature != intake_temp) {
          ESP_LOGD("outequip_ac", "Climate current temp changed to %d C",
                   intake_temp);
          this->current_temperature = intake_temp;
          climate_changed = true;
        }
        break;
      }
      case ACFramer::Key::OutletAirTemp:
        publish_sensor(outlet_temp_sensor_, static_cast<int8_t>(value & 0xFF));
        break;
      case ACFramer::Key::Voltage:
        publish_sensor(voltage_sensor_, value / 10.0f);
        break;
      case ACFramer::Key::LCD:
        if (lcd_switch_ != nullptr &&
            cur_power_state_ == ACFramer::OnOffValue::On) {
          // A serial-interface reported value of 1 is off and 0 is on
          bool is_on = (value == 0);
          if (!lcd_switch_->has_state() || lcd_switch_->state != is_on) {
            ESP_LOGD("outequip_ac", "LCD switch state changed to %s",
                     is_on ? "ON" : "OFF");
            lcd_switch_->publish_state(is_on);
            lcd_switch_->set_has_state(true);
          }
        }
        break;
      case ACFramer::Key::Amperage:
      case ACFramer::Key::Swing:
        break;
      case ACFramer::Key::Light:
        // Ignore reading Light value over serial since the Summit2 firmware is
        // buggy.
        break;
      case ACFramer::Key::Active:
        if (value == 2)
          EnqueueFrame(ACFramer::Key::Active, 1);
        break;
      }

      // Handle Power/Mode combination for Climate
      if (key == ACFramer::Key::Power || key == ACFramer::Key::Mode) {
        climate::ClimateMode new_mode = climate::CLIMATE_MODE_OFF;
        bool preset_changed = false;
        if (cur_power_state_ == ACFramer::OnOffValue::On) {
          switch (cur_mode_) {
          case ACFramer::ModeValue::Cool:
            new_mode = climate::CLIMATE_MODE_COOL;
            preset_changed = this->set_preset_(climate::CLIMATE_PRESET_NONE);
            break;
          case ACFramer::ModeValue::Eco:
            new_mode = climate::CLIMATE_MODE_COOL;
            preset_changed = this->set_preset_(climate::CLIMATE_PRESET_ECO);
            break;
          case ACFramer::ModeValue::Sleep:
            new_mode = climate::CLIMATE_MODE_COOL;
            preset_changed = this->set_preset_(climate::CLIMATE_PRESET_SLEEP);
            break;
          case ACFramer::ModeValue::Turbo:
            new_mode = climate::CLIMATE_MODE_COOL;
            preset_changed = this->set_preset_(climate::CLIMATE_PRESET_BOOST);
            break;
          case ACFramer::ModeValue::Heat:
            new_mode = climate::CLIMATE_MODE_HEAT;
            preset_changed = this->set_preset_(climate::CLIMATE_PRESET_NONE);
            break;
          case ACFramer::ModeValue::Fan:
            new_mode = climate::CLIMATE_MODE_FAN_ONLY;
            preset_changed = this->set_preset_(climate::CLIMATE_PRESET_NONE);
            break;
          default:
            break;
          }
        } else {
          preset_changed = this->set_preset_(climate::CLIMATE_PRESET_NONE);
        }
        if (this->mode != new_mode) {
          ESP_LOGD("outequip_ac", "Climate mode changed to %d",
                   static_cast<int>(new_mode));
          this->mode = new_mode;
          climate_changed = true;
        }
        if (preset_changed) {
          climate_changed = true;
        }
      }

      if (climate_changed) {
        this->publish_state();
      }

      // A set command may reply with a mismatched key on some board firmware.
      // Any valid frame is still a response for pacing purposes.
      if (expecting_key.has_value()) {
        if (*expecting_key != key) {
          ESP_LOGV("outequip_ac", "Expected %s response, received %s",
                   ACFramer::KeyToString(*expecting_key),
                   ACFramer::KeyToString(key));
        }
        expecting_key.reset();
        if (expecting_query_ && key == kQueryKeys[cur_query_key_idx]) {
          if (++cur_query_key_idx >= sizeof(kQueryKeys) / sizeof(*kQueryKeys)) {
            cur_query_key_idx = 0;
            last_full_status = millis();
          }
        }
        expecting_query_ = false;
      }

      rxFramer.Reset();
      MaybeSendCurFrame();
    }
  }
}

climate::ClimateTraits OutEquipAC::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_supported_modes(
      {climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_COOL,
       climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_FAN_ONLY});
  traits.set_supported_presets({climate::CLIMATE_PRESET_NONE,
                                climate::CLIMATE_PRESET_BOOST,
                                climate::CLIMATE_PRESET_SLEEP});
  traits.set_visual_min_temperature(17.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_temperature_step(1.0f);
  return traits;
}

void OutEquipAC::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    auto m = *call.get_mode();
    if (m == climate::CLIMATE_MODE_OFF) {
      EnqueueFrame(ACFramer::Key::Mode, 1);
      EnqueueFrame(ACFramer::Key::Power, 1);
    } else {
      uint16_t ac_mode = 0;
      if (m == climate::CLIMATE_MODE_COOL)
        ac_mode = 1;
      else if (m == climate::CLIMATE_MODE_HEAT)
        ac_mode = 2;
      else if (m == climate::CLIMATE_MODE_FAN_ONLY)
        ac_mode = 3;

      if (ac_mode != 0)
        EnqueueFrame(ACFramer::Key::Mode, ac_mode);
      if (cur_power_state_ != ACFramer::OnOffValue::On)
        EnqueueFrame(ACFramer::Key::Power, 2);
    }
  }

  if (call.get_target_temperature().has_value()) {
    float celsius = esphome::clamp(*call.get_target_temperature(), 17.0f, 30.0f);
    ESP_LOGD("outequip_ac", "Setting target temp to %.1f C using raw UART value %u",
             celsius, static_cast<unsigned>(std::round(celsius)));
    EnqueueFrame(ACFramer::Key::SetTemperature,
                 static_cast<uint16_t>(std::round(celsius)));
  }

  if (call.get_preset().has_value()) {
    auto preset = *call.get_preset();
    uint16_t ac_mode = 0;

    if (preset == climate::CLIMATE_PRESET_NONE) {
      ESP_LOGD("outequip_ac", "Clearing climate preset");
      ac_mode = static_cast<uint16_t>(ACFramer::ModeValue::Cool);
    } else if (preset == climate::CLIMATE_PRESET_ECO) {
      ESP_LOGW("outequip_ac",
               "Eco is not commandable over UART on this board firmware");
    } else if (preset == climate::CLIMATE_PRESET_SLEEP) {
      ESP_LOGD("outequip_ac", "Setting preset to Sleep");
      ac_mode = static_cast<uint16_t>(ACFramer::ModeValue::Sleep);
    } else if (preset == climate::CLIMATE_PRESET_BOOST) {
      ESP_LOGD("outequip_ac", "Setting preset to Boost/Turbo");
      ac_mode = static_cast<uint16_t>(ACFramer::ModeValue::Turbo);
    }

    if (ac_mode != 0) {
      EnqueueFrame(ACFramer::Key::Mode, ac_mode);
      if (cur_power_state_ != ACFramer::OnOffValue::On) {
        EnqueueFrame(ACFramer::Key::Power,
                     static_cast<uint16_t>(ACFramer::OnOffValue::On));
      }
    }
  } else if (call.has_custom_preset()) {
    auto preset = call.get_custom_preset();
    uint16_t ac_mode = 0;

    if (std::strcmp(preset.c_str(), kPresetEco) == 0) {
      ESP_LOGW("outequip_ac",
               "Eco is not commandable over UART on this board firmware");
    } else if (std::strcmp(preset.c_str(), kPresetSleep) == 0) {
      ac_mode = static_cast<uint16_t>(ACFramer::ModeValue::Sleep);
    } else if (std::strcmp(preset.c_str(), kPresetTurbo) == 0) {
      ac_mode = static_cast<uint16_t>(ACFramer::ModeValue::Turbo);
    }

    if (ac_mode != 0) {
      ESP_LOGD("outequip_ac", "Setting custom preset to %s", preset.c_str());
      EnqueueFrame(ACFramer::Key::Mode, ac_mode);
      if (cur_power_state_ != ACFramer::OnOffValue::On) {
        EnqueueFrame(ACFramer::Key::Power,
                     static_cast<uint16_t>(ACFramer::OnOffValue::On));
      }
    }
  }

  if (call.get_fan_mode().has_value()) {
    auto fm = *call.get_fan_mode();
    uint16_t speed = kFanSpeedMedium;
    if (fm == climate::CLIMATE_FAN_AUTO)
      speed = kFanSpeedAuto;
    if (fm == climate::CLIMATE_FAN_LOW)
      speed = kFanSpeedLow;
    else if (fm == climate::CLIMATE_FAN_MEDIUM)
      speed = kFanSpeedMedium;
    else if (fm == climate::CLIMATE_FAN_HIGH)
      speed = kFanSpeedHigh;
    EnqueueFrame(ACFramer::Key::FanSpeed, speed);
  }

  if (call.has_custom_fan_mode()) {
    auto fm = call.get_custom_fan_mode();
    const auto speed = CustomFanModeToFanValue(fm.c_str());
    if (speed != 0) {
      ESP_LOGD("outequip_ac", "Setting custom fan mode to %s", fm.c_str());
      EnqueueFrame(ACFramer::Key::FanSpeed, speed);
    } else {
      ESP_LOGW("outequip_ac", "Unsupported custom fan mode %s", fm.c_str());
    }
  }
}

void OutEquipAC::WriteFrame(ACFramer &framer) {
  expecting_key = framer.GetKey();
  expecting_query_ = framer.GetValue() == ACFramer::kQueryVal;
  this->write_array(framer.buffer(), framer.buffer_pos());
  last_frame_sent = millis();
  num_frames_tx_++;
  PublishCounterSensors();
}

void OutEquipAC::MaybeSendCurFrame() {
  if (expecting_key.has_value()) {
    if (millis() - last_frame_sent < kResponseTimeoutMs) {
      return;
    }
    ESP_LOGW("outequip_ac", "Timed out waiting for %s response",
             ACFramer::KeyToString(*expecting_key));
    expecting_key.reset();
    expecting_query_ = false;
  }

  if (!txQueue.empty()) {
    WriteFrame(txQueue.front());
    txQueue.pop();
    return;
  }
  if (millis() - last_full_status < 1000) {
    return;
  }
  ACFramer txFramer;
  txFramer.NewFrame(kQueryKeys[cur_query_key_idx], ACFramer::kQueryVal);
  WriteFrame(txFramer);
}

bool OutEquipAC::EnqueueFrame(ACFramer::Key key, uint16_t value) {
  ACFramer txFramer;
  if (!txFramer.NewFrame(key, value))
    return false;
  txQueue.push(txFramer);
  return true;
}

bool OutEquipAC::EnqueueRawFrame(uint8_t key, uint16_t value) {
  if (!ACFramer::ValidateKey(key)) {
    return false;
  }
  return EnqueueFrame(static_cast<ACFramer::Key>(key), value);
}

const char *OutEquipAC::FanValueToCustomFanMode(uint16_t value) {
  switch (value) {
  case 1:
    return kFanMode1;
  case 2:
    return kFanMode2;
  case 3:
    return kFanMode3;
  case 4:
    return kFanMode4;
  case 5:
    return kFanMode5Auto;
  default:
    return nullptr;
  }
}

uint16_t OutEquipAC::CustomFanModeToFanValue(const char *mode) {
  if (std::strcmp(mode, kFanMode1) == 0)
    return 1;
  if (std::strcmp(mode, kFanMode2) == 0)
    return 2;
  if (std::strcmp(mode, kFanMode3) == 0)
    return 3;
  if (std::strcmp(mode, kFanMode4) == 0)
    return 4;
  if (std::strcmp(mode, kFanMode5Auto) == 0)
    return 5;
  return 0;
}

void OutEquipAC::PublishCounterSensors() {
  if (frames_tx_sensor_ != nullptr) {
    frames_tx_sensor_->publish_state(num_frames_tx_);
  }
  if (frames_rx_sensor_ != nullptr) {
    frames_rx_sensor_->publish_state(num_frames_rx_);
  }
  if (frames_failed_sensor_ != nullptr) {
    frames_failed_sensor_->publish_state(num_frames_failed_);
  }
  if (spurious_bytes_rx_sensor_ != nullptr) {
    spurious_bytes_rx_sensor_->publish_state(num_spurious_bytes_rx_);
  }
}

void OutEquipAC::PublishRawSensor(ACFramer::Key key, uint16_t value) {
  sensor::Sensor *sensor = nullptr;
  switch (key) {
  case ACFramer::Key::Power:
    sensor = raw_power_sensor_;
    break;
  case ACFramer::Key::Mode:
    sensor = raw_mode_sensor_;
    break;
  case ACFramer::Key::SetTemperature:
    sensor = raw_set_temperature_sensor_;
    break;
  case ACFramer::Key::FanSpeed:
    sensor = raw_fan_speed_sensor_;
    break;
  case ACFramer::Key::UndervoltProtect:
    sensor = raw_undervolt_sensor_;
    break;
  case ACFramer::Key::OvervoltProtect:
    sensor = raw_overvolt_sensor_;
    break;
  case ACFramer::Key::IntakeAirTemp:
    sensor = raw_intake_temp_sensor_;
    break;
  case ACFramer::Key::OutletAirTemp:
    sensor = raw_outlet_temp_sensor_;
    break;
  case ACFramer::Key::LCD:
    sensor = raw_lcd_sensor_;
    break;
  case ACFramer::Key::Swing:
    sensor = raw_swing_sensor_;
    break;
  case ACFramer::Key::Voltage:
    sensor = raw_voltage_sensor_;
    break;
  case ACFramer::Key::Amperage:
    sensor = raw_amperage_sensor_;
    break;
  case ACFramer::Key::Light:
    sensor = raw_light_sensor_;
    break;
  case ACFramer::Key::Active:
    sensor = raw_active_sensor_;
    break;
  }

  if (sensor != nullptr && (!sensor->has_state() || sensor->state != value)) {
    sensor->publish_state(value);
  }
}

constexpr ACFramer::Key OutEquipAC::kQueryKeys[];

} // namespace outequip_ac
} // namespace esphome
