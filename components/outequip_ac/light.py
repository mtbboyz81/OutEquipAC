import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_OUTPUT_ID

from . import CONF_OUTEQUIP_AC_ID, OutEquipAC, outequip_ac_ns

DEPENDENCIES = ["outequip_ac"]

OutEquipACLight = outequip_ac_ns.class_("OutEquipACLight", light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.light_schema(
    OutEquipACLight,
    light.LightType.BINARY,
    icon="mdi:wall-sconce-flat",
    default_restore_mode="RESTORE_DEFAULT_OFF",
).extend({
    cv.GenerateID(CONF_OUTEQUIP_AC_ID): cv.use_id(OutEquipAC),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_OUTEQUIP_AC_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.set_light_output(var))
    await light.register_light(var, config)
