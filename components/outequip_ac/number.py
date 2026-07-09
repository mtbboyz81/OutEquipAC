import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_VOLT,
)

from . import CONF_OUTEQUIP_AC_ID, OutEquipAC, outequip_ac_ns

DEPENDENCIES = ["outequip_ac"]

CONF_UNDERVOLT = "undervolt"
CONF_RAW_KEY = "raw_key"
CONF_RAW_VALUE = "raw_value"
UNDERVOLT_MIN = 9.0
UNDERVOLT_MAX = 11.9
UNDERVOLT_STEP = 0.1

OutEquipACNumber = outequip_ac_ns.class_("OutEquipACNumber", number.Number, cg.Component)
OutEquipACNumberType = outequip_ac_ns.enum("OutEquipACNumberType")

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OUTEQUIP_AC_ID): cv.use_id(OutEquipAC),
    cv.Optional(CONF_UNDERVOLT): number.number_schema(
        OutEquipACNumber,
        device_class=DEVICE_CLASS_VOLTAGE,
        entity_category=ENTITY_CATEGORY_CONFIG,
        unit_of_measurement=UNIT_VOLT,
        icon="mdi:battery-low",
    ).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_RAW_KEY): number.number_schema(
        OutEquipACNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:key-variant",
    ).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_RAW_VALUE): number.number_schema(
        OutEquipACNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:numeric",
    ).extend(cv.COMPONENT_SCHEMA),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_OUTEQUIP_AC_ID])

    if CONF_UNDERVOLT in config:
        conf = config[CONF_UNDERVOLT]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await number.register_number(
            var,
            conf,
            min_value=UNDERVOLT_MIN,
            max_value=UNDERVOLT_MAX,
            step=UNDERVOLT_STEP,
        )
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(OutEquipACNumberType.UNDERVOLT))
        cg.add(parent.set_undervolt_number(var))

    if CONF_RAW_KEY in config:
        conf = config[CONF_RAW_KEY]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await number.register_number(var, conf, min_value=1, max_value=255, step=1)
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(OutEquipACNumberType.RAW_KEY))

    if CONF_RAW_VALUE in config:
        conf = config[CONF_RAW_VALUE]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await number.register_number(var, conf, min_value=0, max_value=65535, step=1)
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(OutEquipACNumberType.RAW_VALUE))
