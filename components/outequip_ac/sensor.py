import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CELSIUS,
    UNIT_VOLT,
)
from . import outequip_ac_ns, OutEquipAC, CONF_OUTEQUIP_AC_ID

DEPENDENCIES = ["outequip_ac"]

CONF_INTAKE_TEMP = "intake_temp"
CONF_OUTLET_TEMP = "outlet_temp"
CONF_VOLTAGE = "voltage"
CONF_UNDERVOLT = "undervolt"
CONF_OVERVOLT = "overvolt"
CONF_FRAMES_TX = "frames_tx"
CONF_FRAMES_RX = "frames_rx"
CONF_FRAMES_FAILED = "frames_failed"
CONF_SPURIOUS_BYTES_RX = "spurious_bytes_rx"
CONF_RAW_POWER = "raw_power"
CONF_RAW_MODE = "raw_mode"
CONF_RAW_SET_TEMPERATURE = "raw_set_temperature"
CONF_RAW_FAN_SPEED = "raw_fan_speed"
CONF_RAW_UNDERVOLT = "raw_undervolt"
CONF_RAW_OVERVOLT = "raw_overvolt"
CONF_RAW_INTAKE_TEMP = "raw_intake_temp"
CONF_RAW_OUTLET_TEMP = "raw_outlet_temp"
CONF_RAW_LCD = "raw_lcd"
CONF_RAW_SWING = "raw_swing"
CONF_RAW_VOLTAGE = "raw_voltage"
CONF_RAW_AMPERAGE = "raw_amperage"
CONF_RAW_LIGHT = "raw_light"
CONF_RAW_ACTIVE = "raw_active"


def raw_sensor_schema(icon):
    return sensor.sensor_schema(
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        state_class=STATE_CLASS_MEASUREMENT,
        icon=icon,
    )

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OUTEQUIP_AC_ID): cv.use_id(OutEquipAC),
    cv.Optional(CONF_INTAKE_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:import",
    ),
    cv.Optional(CONF_OUTLET_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:export",
    ),
    cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:flash",
    ),
    cv.Optional(CONF_UNDERVOLT): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:battery-low",
    ),
    cv.Optional(CONF_OVERVOLT): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:battery-alert-variant",
    ),
    cv.Optional(CONF_FRAMES_TX): sensor.sensor_schema(
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:upload-network",
    ),
    cv.Optional(CONF_FRAMES_RX): sensor.sensor_schema(
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:download-network",
    ),
    cv.Optional(CONF_FRAMES_FAILED): sensor.sensor_schema(
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:network-off",
    ),
    cv.Optional(CONF_SPURIOUS_BYTES_RX): sensor.sensor_schema(
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        icon="mdi:alert-octagon-outline",
    ),
    cv.Optional(CONF_RAW_POWER): raw_sensor_schema("mdi:power"),
    cv.Optional(CONF_RAW_MODE): raw_sensor_schema("mdi:tune-variant"),
    cv.Optional(CONF_RAW_SET_TEMPERATURE): raw_sensor_schema("mdi:thermometer"),
    cv.Optional(CONF_RAW_FAN_SPEED): raw_sensor_schema("mdi:fan"),
    cv.Optional(CONF_RAW_UNDERVOLT): raw_sensor_schema("mdi:battery-low"),
    cv.Optional(CONF_RAW_OVERVOLT): raw_sensor_schema("mdi:battery-alert-variant"),
    cv.Optional(CONF_RAW_INTAKE_TEMP): raw_sensor_schema("mdi:import"),
    cv.Optional(CONF_RAW_OUTLET_TEMP): raw_sensor_schema("mdi:export"),
    cv.Optional(CONF_RAW_LCD): raw_sensor_schema("mdi:clock-digital"),
    cv.Optional(CONF_RAW_SWING): raw_sensor_schema("mdi:swap-vertical"),
    cv.Optional(CONF_RAW_VOLTAGE): raw_sensor_schema("mdi:flash"),
    cv.Optional(CONF_RAW_AMPERAGE): raw_sensor_schema("mdi:current-dc"),
    cv.Optional(CONF_RAW_LIGHT): raw_sensor_schema("mdi:wall-sconce-flat"),
    cv.Optional(CONF_RAW_ACTIVE): raw_sensor_schema("mdi:connection"),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_OUTEQUIP_AC_ID])
    
    if CONF_INTAKE_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_INTAKE_TEMP])
        cg.add(parent.set_intake_temp_sensor(sens))
        
    if CONF_OUTLET_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_OUTLET_TEMP])
        cg.add(parent.set_outlet_temp_sensor(sens))
        
    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(parent.set_voltage_sensor(sens))
        
    if CONF_UNDERVOLT in config:
        sens = await sensor.new_sensor(config[CONF_UNDERVOLT])
        cg.add(parent.set_undervolt_sensor(sens))

    if CONF_OVERVOLT in config:
        sens = await sensor.new_sensor(config[CONF_OVERVOLT])
        cg.add(parent.set_overvolt_sensor(sens))

    if CONF_FRAMES_TX in config:
        sens = await sensor.new_sensor(config[CONF_FRAMES_TX])
        cg.add(parent.set_frames_tx_sensor(sens))

    if CONF_FRAMES_RX in config:
        sens = await sensor.new_sensor(config[CONF_FRAMES_RX])
        cg.add(parent.set_frames_rx_sensor(sens))

    if CONF_FRAMES_FAILED in config:
        sens = await sensor.new_sensor(config[CONF_FRAMES_FAILED])
        cg.add(parent.set_frames_failed_sensor(sens))

    if CONF_SPURIOUS_BYTES_RX in config:
        sens = await sensor.new_sensor(config[CONF_SPURIOUS_BYTES_RX])
        cg.add(parent.set_spurious_bytes_rx_sensor(sens))

    if CONF_RAW_POWER in config:
        sens = await sensor.new_sensor(config[CONF_RAW_POWER])
        cg.add(parent.set_raw_power_sensor(sens))

    if CONF_RAW_MODE in config:
        sens = await sensor.new_sensor(config[CONF_RAW_MODE])
        cg.add(parent.set_raw_mode_sensor(sens))

    if CONF_RAW_SET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_RAW_SET_TEMPERATURE])
        cg.add(parent.set_raw_set_temperature_sensor(sens))

    if CONF_RAW_FAN_SPEED in config:
        sens = await sensor.new_sensor(config[CONF_RAW_FAN_SPEED])
        cg.add(parent.set_raw_fan_speed_sensor(sens))

    if CONF_RAW_UNDERVOLT in config:
        sens = await sensor.new_sensor(config[CONF_RAW_UNDERVOLT])
        cg.add(parent.set_raw_undervolt_sensor(sens))

    if CONF_RAW_OVERVOLT in config:
        sens = await sensor.new_sensor(config[CONF_RAW_OVERVOLT])
        cg.add(parent.set_raw_overvolt_sensor(sens))

    if CONF_RAW_INTAKE_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_RAW_INTAKE_TEMP])
        cg.add(parent.set_raw_intake_temp_sensor(sens))

    if CONF_RAW_OUTLET_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_RAW_OUTLET_TEMP])
        cg.add(parent.set_raw_outlet_temp_sensor(sens))

    if CONF_RAW_LCD in config:
        sens = await sensor.new_sensor(config[CONF_RAW_LCD])
        cg.add(parent.set_raw_lcd_sensor(sens))

    if CONF_RAW_SWING in config:
        sens = await sensor.new_sensor(config[CONF_RAW_SWING])
        cg.add(parent.set_raw_swing_sensor(sens))

    if CONF_RAW_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_RAW_VOLTAGE])
        cg.add(parent.set_raw_voltage_sensor(sens))

    if CONF_RAW_AMPERAGE in config:
        sens = await sensor.new_sensor(config[CONF_RAW_AMPERAGE])
        cg.add(parent.set_raw_amperage_sensor(sens))

    if CONF_RAW_LIGHT in config:
        sens = await sensor.new_sensor(config[CONF_RAW_LIGHT])
        cg.add(parent.set_raw_light_sensor(sens))

    if CONF_RAW_ACTIVE in config:
        sens = await sensor.new_sensor(config[CONF_RAW_ACTIVE])
        cg.add(parent.set_raw_active_sensor(sens))
