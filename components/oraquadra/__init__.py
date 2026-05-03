"""ESPHome external component glue for OraQuadra.

Wires the YAML config block into the C++ ``OraquadraComponent``. The component
needs three references at minimum (LED strip, time source) plus optional
BME680 sensors for the IAQ frame.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, sensor, time as time_
from esphome.const import CONF_ID, CONF_TIME_ID

CODEOWNERS = ["@manucappelleri"]
DEPENDENCIES = ["light", "time"]
AUTO_LOAD = ["json"]

oraquadra_ns = cg.esphome_ns.namespace("oraquadra")
OraquadraComponent = oraquadra_ns.class_("OraquadraComponent", cg.Component)

CONF_LIGHT_ID = "light_id"
CONF_IAQ_SENSOR_ID = "iaq_sensor_id"
CONF_IAQ_ACCURACY_SENSOR_ID = "iaq_accuracy_sensor_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OraquadraComponent),
        cv.Required(CONF_LIGHT_ID): cv.use_id(light.AddressableLightState),
        cv.Required(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
        cv.Optional(CONF_IAQ_SENSOR_ID): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_IAQ_ACCURACY_SENSOR_ID): cv.use_id(sensor.Sensor),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    light_var = await cg.get_variable(config[CONF_LIGHT_ID])
    cg.add(var.set_light(light_var))

    time_var = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_var))

    if CONF_IAQ_SENSOR_ID in config:
        iaq_var = await cg.get_variable(config[CONF_IAQ_SENSOR_ID])
        cg.add(var.set_iaq_sensor(iaq_var))

    if CONF_IAQ_ACCURACY_SENSOR_ID in config:
        acc_var = await cg.get_variable(config[CONF_IAQ_ACCURACY_SENSOR_ID])
        cg.add(var.set_iaq_accuracy_sensor(acc_var))
