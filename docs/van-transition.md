# Van ESPHome Transition

Goal: move `outequip-ac-d1mini` from the home network/Home Assistant ESPHome
instance to the van network/Home Assistant ESPHome instance without stranding
the D1 Mini.

## Current Important State

- Device name: `outequip-ac-d1mini`
- Current home IP observed during testing: `192.168.1.169`
- MAC: `E8:DB:84:9C:2D:90`
- The local repo YAML currently has no `wifi.ssid` / `wifi.password`; the
  device is relying on stored WiFi credentials/captive portal behavior.
- UART must stay on GPIO1 TX / GPIO3 RX at 115200.
- Logger must keep `baud_rate: 0`.
- The van ESPHome dashboard needs the custom component files under:

```text
/config/component/outequip_ac/
```

## Recommended Migration Path

1. Copy the custom component folder to the van Home Assistant instance:

```text
components/outequip_ac/__init__.py
components/outequip_ac/ac_framer.cpp
components/outequip_ac/ac_framer.h
components/outequip_ac/button.py
components/outequip_ac/climate.py
components/outequip_ac/light.py
components/outequip_ac/number.py
components/outequip_ac/outequip_ac.cpp
components/outequip_ac/outequip_ac.h
components/outequip_ac/sensor.py
components/outequip_ac/switch.py
```

2. In the van Home Assistant ESPHome dashboard, create a device YAML named
   `outequip-ac-d1mini.yaml` using the YAML below.

3. Add these secrets to the van Home Assistant `secrets.yaml`:

```yaml
wifi_ssid: "VAN_WIFI_NAME"
wifi_password: "VAN_WIFI_PASSWORD"
```

4. Move the D1 Mini into the van and power it up.

5. If it does not join the van WiFi automatically, connect to its fallback AP:

```text
OutEquip AC Fallback
```

Then use the captive portal to set the van WiFi credentials.

6. Once the device is visible on the van network, install from the van ESPHome
   dashboard. This makes the van Home Assistant instance the source of truth.

## Van ESPHome YAML

```yaml
substitutions:
  name: outequip-ac-d1mini
  friendly_name: OutEquip AC

esphome:
  name: ${name}
  friendly_name: ${friendly_name}
  name_add_mac_suffix: false
  project:
    name: "gongloo.${name}"
    version: "2.1.4-d1mini"
  min_version: 2026.4.5

esp8266:
  board: d1_mini

external_components:
  - source:
      type: local
      path: /config/component
    components: [outequip_ac]

logger:
  level: DEBUG
  baud_rate: 0

api:
  reboot_timeout: 0s

ota:
  - platform: esphome

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "${friendly_name} Fallback"

captive_portal:

uart:
  id: uart_bus
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 115200
  rx_buffer_size: 512

outequip_ac:
  id: ac_device
  uart_id: uart_bus

climate:
  - platform: outequip_ac
    outequip_ac_id: ac_device
    name: "Thermostat"
    id: ac_climate
    visual:
      min_temperature: 17
      max_temperature: 30
      temperature_step:
        target_temperature: 1
        current_temperature: 1

switch:
  - platform: outequip_ac
    outequip_ac_id: ac_device
    lcd:
      name: "LCD Backlight"
      id: ac_lcd
      restore_mode: RESTORE_DEFAULT_ON

light:
  - platform: outequip_ac
    outequip_ac_id: ac_device
    name: "Light"
    id: ac_light

sensor:
  - platform: outequip_ac
    outequip_ac_id: ac_device
    intake_temp:
      name: "Intake Air Temp"
      id: ac_intake_temp
    outlet_temp:
      name: "Outlet Air Temp"
      id: ac_outlet_temp
    voltage:
      name: "Voltage"
      id: ac_voltage
    undervolt:
      name: "Undervolt Protect"
      id: ac_undervolt
    overvolt:
      name: "Overvolt Protect"
      id: ac_overvolt
    frames_tx:
      name: "UART Frames TX"
      id: ac_frames_tx
    frames_rx:
      name: "UART Frames RX"
      id: ac_frames_rx
    frames_failed:
      name: "UART Frames Failed"
      id: ac_frames_failed
    spurious_bytes_rx:
      name: "UART Spurious Bytes RX"
      id: ac_spurious_bytes_rx
    raw_power:
      name: "Raw UART Power"
      id: ac_raw_power
    raw_mode:
      name: "Raw UART Mode"
      id: ac_raw_mode
    raw_set_temperature:
      name: "Raw UART Set Temperature"
      id: ac_raw_set_temperature
    raw_fan_speed:
      name: "Raw UART Fan Speed"
      id: ac_raw_fan_speed
    raw_undervolt:
      name: "Raw UART Undervolt"
      id: ac_raw_undervolt
    raw_overvolt:
      name: "Raw UART Overvolt"
      id: ac_raw_overvolt
    raw_intake_temp:
      name: "Raw UART Intake Temp"
      id: ac_raw_intake_temp
    raw_outlet_temp:
      name: "Raw UART Outlet Temp"
      id: ac_raw_outlet_temp
    raw_lcd:
      name: "Raw UART LCD"
      id: ac_raw_lcd
    raw_swing:
      name: "Raw UART Swing"
      id: ac_raw_swing
    raw_voltage:
      name: "Raw UART Voltage"
      id: ac_raw_voltage
    raw_amperage:
      name: "Raw UART Amperage"
      id: ac_raw_amperage
    raw_light:
      name: "Raw UART Light"
      id: ac_raw_light
    raw_active:
      name: "Raw UART Active"
      id: ac_raw_active

number:
  - platform: outequip_ac
    outequip_ac_id: ac_device
    undervolt:
      name: "Undervolt Cutoff"
      id: ac_undervolt_cutoff
    raw_key:
      name: "Raw UART Key"
      id: ac_raw_key
    raw_value:
      name: "Raw UART Value"
      id: ac_raw_value

button:
  - platform: outequip_ac
    outequip_ac_id: ac_device
    raw_send:
      name: "Raw UART Send"
      id: ac_raw_send
    raw_query:
      name: "Raw UART Query"
      id: ac_raw_query

  - platform: restart
    name: "Restart"
    id: restart_button
    entity_category: diagnostic
```

## Notes

- Do not change `name` during migration unless you intentionally want a new
  mDNS/API identity.
- Keep the same `name` so Home Assistant can rediscover the same API device on
  the van network.
- If the device fails to join van WiFi, use the fallback AP/captive portal
  rather than reflashing blindly.
