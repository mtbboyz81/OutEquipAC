#pragma once

#include "ac_framer.h"
#include "esphome/components/button/button.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/number/number.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include <optional>
#include <queue>

namespace esphome {
namespace outequip_ac {

static constexpr const char *kPresetEco = "Eco";
static constexpr const char *kPresetSleep = "Sleep";
static constexpr const char *kPresetTurbo = "Turbo";
static constexpr const char *kFanMode1 = "Fan 1";
static constexpr const char *kFanMode2 = "Fan 2";
static constexpr const char *kFanMode3 = "Fan 3";
static constexpr const char *kFanMode4 = "Fan 4";
static constexpr const char *kFanMode5Auto = "Fan 5 Auto";
static constexpr uint16_t kFanSpeedLow = 1;
static constexpr uint16_t kFanSpeedMedium = 3;
static constexpr uint16_t kFanSpeedHigh = 4;
static constexpr uint16_t kFanSpeedAuto = 5;
static constexpr float kUndervoltProtectMinVolts = 9.0f;
static constexpr float kUndervoltProtectMaxVolts = 11.9f;

class OutEquipAC;

enum OutEquipACButtonType { RAW_SEND, RAW_QUERY };
enum OutEquipACNumberType { UNDERVOLT, RAW_KEY, RAW_VALUE };

class OutEquipACSwitch : public switch_::Switch, public Component {
public:
  void set_parent(OutEquipAC *parent) { parent_ = parent; }
  void write_state(bool state) override;
  void setup() override;

protected:
  OutEquipAC *parent_{nullptr};
};

class OutEquipACLight : public light::LightOutput, public Component {
public:
  void set_parent(OutEquipAC *parent) { parent_ = parent; }
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

protected:
  OutEquipAC *parent_{nullptr};
};

class OutEquipACNumber : public number::Number, public Component {
public:
  void set_parent(OutEquipAC *parent) { parent_ = parent; }
  void set_type(OutEquipACNumberType type) { type_ = type; }

protected:
  void control(float value) override;

private:
  OutEquipAC *parent_{nullptr};
  OutEquipACNumberType type_{UNDERVOLT};
};

class OutEquipACButton : public button::Button, public Component {
public:
  void set_parent(OutEquipAC *parent) { parent_ = parent; }
  void set_type(OutEquipACButtonType type) { type_ = type; }

protected:
  void press_action() override;

private:
  OutEquipAC *parent_{nullptr};
  OutEquipACButtonType type_{RAW_SEND};
};

class OutEquipAC : public Component,
                   public climate::Climate,
                   public uart::UARTDevice {
public:
  OutEquipAC() = default;

  void set_intake_temp_sensor(sensor::Sensor *sensor) {
    intake_temp_sensor_ = sensor;
  }
  void set_outlet_temp_sensor(sensor::Sensor *sensor) {
    outlet_temp_sensor_ = sensor;
  }
  void set_voltage_sensor(sensor::Sensor *sensor) { voltage_sensor_ = sensor; }
  void set_undervolt_sensor(sensor::Sensor *sensor) {
    undervolt_sensor_ = sensor;
  }
  void set_overvolt_sensor(sensor::Sensor *sensor) {
    overvolt_sensor_ = sensor;
  }
  void set_frames_tx_sensor(sensor::Sensor *sensor) { frames_tx_sensor_ = sensor; }
  void set_frames_rx_sensor(sensor::Sensor *sensor) { frames_rx_sensor_ = sensor; }
  void set_frames_failed_sensor(sensor::Sensor *sensor) {
    frames_failed_sensor_ = sensor;
  }
  void set_spurious_bytes_rx_sensor(sensor::Sensor *sensor) {
    spurious_bytes_rx_sensor_ = sensor;
  }
  void set_raw_power_sensor(sensor::Sensor *sensor) { raw_power_sensor_ = sensor; }
  void set_raw_mode_sensor(sensor::Sensor *sensor) { raw_mode_sensor_ = sensor; }
  void set_raw_set_temperature_sensor(sensor::Sensor *sensor) {
    raw_set_temperature_sensor_ = sensor;
  }
  void set_raw_fan_speed_sensor(sensor::Sensor *sensor) {
    raw_fan_speed_sensor_ = sensor;
  }
  void set_raw_undervolt_sensor(sensor::Sensor *sensor) {
    raw_undervolt_sensor_ = sensor;
  }
  void set_raw_overvolt_sensor(sensor::Sensor *sensor) {
    raw_overvolt_sensor_ = sensor;
  }
  void set_raw_intake_temp_sensor(sensor::Sensor *sensor) {
    raw_intake_temp_sensor_ = sensor;
  }
  void set_raw_outlet_temp_sensor(sensor::Sensor *sensor) {
    raw_outlet_temp_sensor_ = sensor;
  }
  void set_raw_lcd_sensor(sensor::Sensor *sensor) { raw_lcd_sensor_ = sensor; }
  void set_raw_swing_sensor(sensor::Sensor *sensor) { raw_swing_sensor_ = sensor; }
  void set_raw_voltage_sensor(sensor::Sensor *sensor) {
    raw_voltage_sensor_ = sensor;
  }
  void set_raw_amperage_sensor(sensor::Sensor *sensor) {
    raw_amperage_sensor_ = sensor;
  }
  void set_raw_light_sensor(sensor::Sensor *sensor) { raw_light_sensor_ = sensor; }
  void set_raw_active_sensor(sensor::Sensor *sensor) {
    raw_active_sensor_ = sensor;
  }
  void set_lcd_switch(switch_::Switch *lcd_switch) { lcd_switch_ = lcd_switch; }
  void set_light_output(OutEquipACLight *light_output) {
    light_output_ = light_output;
  }
  void set_undervolt_number(number::Number *number) {
    undervolt_number_ = number;
  }

  void set_lcd_state(bool state);
  void set_light_state(bool state);
  void set_undervolt_protect(float voltage);
  void set_raw_command_key(float key);
  void set_raw_command_value(float value);
  void send_raw_command_value();
  void query_raw_command_key();

  void setup() override;
  void loop() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  ACFramer::OnOffValue power_state() const { return cur_power_state_; }
  ACFramer::ModeValue cur_mode() const { return cur_mode_; }
  uint16_t fan_speed() const { return cur_fan_speed_; }
  uint32_t num_frames_tx() const { return num_frames_tx_; }
  uint32_t num_frames_rx() const { return num_frames_rx_; }
  uint32_t num_frames_failed() const { return num_frames_failed_; }
  uint32_t num_spurious_bytes_rx() const { return num_spurious_bytes_rx_; }

protected:
  sensor::Sensor *intake_temp_sensor_{nullptr};
  sensor::Sensor *outlet_temp_sensor_{nullptr};
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *undervolt_sensor_{nullptr};
  sensor::Sensor *overvolt_sensor_{nullptr};
  sensor::Sensor *frames_tx_sensor_{nullptr};
  sensor::Sensor *frames_rx_sensor_{nullptr};
  sensor::Sensor *frames_failed_sensor_{nullptr};
  sensor::Sensor *spurious_bytes_rx_sensor_{nullptr};
  sensor::Sensor *raw_power_sensor_{nullptr};
  sensor::Sensor *raw_mode_sensor_{nullptr};
  sensor::Sensor *raw_set_temperature_sensor_{nullptr};
  sensor::Sensor *raw_fan_speed_sensor_{nullptr};
  sensor::Sensor *raw_undervolt_sensor_{nullptr};
  sensor::Sensor *raw_overvolt_sensor_{nullptr};
  sensor::Sensor *raw_intake_temp_sensor_{nullptr};
  sensor::Sensor *raw_outlet_temp_sensor_{nullptr};
  sensor::Sensor *raw_lcd_sensor_{nullptr};
  sensor::Sensor *raw_swing_sensor_{nullptr};
  sensor::Sensor *raw_voltage_sensor_{nullptr};
  sensor::Sensor *raw_amperage_sensor_{nullptr};
  sensor::Sensor *raw_light_sensor_{nullptr};
  sensor::Sensor *raw_active_sensor_{nullptr};
  switch_::Switch *lcd_switch_{nullptr};
  OutEquipACLight *light_output_{nullptr};
  number::Number *undervolt_number_{nullptr};

private:
  void WriteFrame(ACFramer &framer);
  void MaybeSendCurFrame();
  bool EnqueueFrame(ACFramer::Key key, uint16_t value);
  void PublishCounterSensors();
  void PublishRawSensor(ACFramer::Key key, uint16_t value);
  bool EnqueueRawFrame(uint8_t key, uint16_t value);
  static const char *FanValueToCustomFanMode(uint16_t value);
  static uint16_t CustomFanModeToFanValue(const char *mode);

  static constexpr uint32_t kFrameIntervalMs = 1000;
  static constexpr uint32_t kResponseTimeoutMs = 1500;

  constexpr static ACFramer::Key kQueryKeys[] = {
      ACFramer::Key::Power,
      ACFramer::Key::Mode,
      ACFramer::Key::SetTemperature,
      ACFramer::Key::FanSpeed,
      ACFramer::Key::UndervoltProtect,
      ACFramer::Key::OvervoltProtect,
      ACFramer::Key::IntakeAirTemp,
      ACFramer::Key::OutletAirTemp,
      // Summit2 firmware has light/lcd status reporting is buggy. Ignore.
      ACFramer::Key::LCD,
      // ACFramer::Key::Light,
      ACFramer::Key::Voltage,
  };

  size_t cur_query_key_idx = 0;
  uint32_t last_frame_sent = 0;
  uint32_t last_full_status = 0;
  std::queue<ACFramer> txQueue;
  std::optional<ACFramer::Key> expecting_key;
  bool expecting_query_{false};
  ACFramer rxFramer;

  ACFramer::OnOffValue cur_power_state_ = ACFramer::OnOffValue::Query;
  ACFramer::ModeValue cur_mode_ = ACFramer::ModeValue::Query;
  uint16_t cur_fan_speed_{0};
  uint8_t raw_command_key_{static_cast<uint8_t>(ACFramer::Key::Mode)};
  uint16_t raw_command_value_{ACFramer::kQueryVal};
  uint32_t num_frames_tx_{0};
  uint32_t num_frames_rx_{0};
  uint32_t num_frames_failed_{0};
  uint32_t num_spurious_bytes_rx_{0};
};

} // namespace outequip_ac
} // namespace esphome
