"""ESPHome external component glue for OraQuadra.

Wires the YAML config block into the C++ ``OraquadraComponent``. The component
needs three references at minimum (LED strip, time source) plus optional
BME680 sensors for the IAQ frame.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, time as time_
from esphome.const import CONF_ID, CONF_TIME_ID

CODEOWNERS = ["@manucappelleri"]
DEPENDENCIES = ["light", "time"]
AUTO_LOAD = ["json"]

oraquadra_ns = cg.esphome_ns.namespace("oraquadra")
OraquadraComponent = oraquadra_ns.class_("OraquadraComponent", cg.Component)

CONF_LIGHT_ID = "light_id"

# IAQ values flow IN via the BME680 sensor's on_value lambda calling
# `id(oraquadra_component)->set_iaq(x, accuracy)`. We deliberately do NOT
# take the iaq sensor as a config dependency here, because that would create
# a circular dependency:
#     oraquadra → needs env_iaq_accuracy variable
#     env_iaq_accuracy.on_value → calls id(oraquadra_component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OraquadraComponent),
        cv.Required(CONF_LIGHT_ID): cv.use_id(light.AddressableLightState),
        cv.Required(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    light_var = await cg.get_variable(config[CONF_LIGHT_ID])
    cg.add(var.set_light(light_var))

    time_var = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_var))
