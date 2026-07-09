import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_ID, ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_OUTEQUIP_AC_ID, OutEquipAC, outequip_ac_ns

DEPENDENCIES = ["outequip_ac"]

CONF_RAW_SEND = "raw_send"
CONF_RAW_QUERY = "raw_query"

OutEquipACButton = outequip_ac_ns.class_("OutEquipACButton", button.Button, cg.Component)
OutEquipACButtonType = outequip_ac_ns.enum("OutEquipACButtonType")

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OUTEQUIP_AC_ID): cv.use_id(OutEquipAC),
    cv.Optional(CONF_RAW_SEND): button.button_schema(
        OutEquipACButton,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:send",
    ).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_RAW_QUERY): button.button_schema(
        OutEquipACButton,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:database-search",
    ).extend(cv.COMPONENT_SCHEMA),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_OUTEQUIP_AC_ID])

    if CONF_RAW_SEND in config:
        conf = config[CONF_RAW_SEND]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await button.register_button(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(OutEquipACButtonType.RAW_SEND))

    if CONF_RAW_QUERY in config:
        conf = config[CONF_RAW_QUERY]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await button.register_button(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(OutEquipACButtonType.RAW_QUERY))
