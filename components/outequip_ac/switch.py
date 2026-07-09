import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_ICON
from . import outequip_ac_ns, OutEquipAC, CONF_OUTEQUIP_AC_ID

DEPENDENCIES = ["outequip_ac"]

CONF_LCD = "lcd"

OutEquipACSwitch = outequip_ac_ns.class_("OutEquipACSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_OUTEQUIP_AC_ID): cv.use_id(OutEquipAC),
    cv.Optional(CONF_LCD): switch.switch_schema(
        OutEquipACSwitch,
        default_restore_mode="RESTORE_DEFAULT_ON",
        icon="mdi:clock-digital",
    ).extend(cv.COMPONENT_SCHEMA),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_OUTEQUIP_AC_ID])
    
    if CONF_LCD in config:
        conf = config[CONF_LCD]
        var = await switch.new_switch(conf)
        await cg.register_component(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(parent.set_lcd_switch(var))
