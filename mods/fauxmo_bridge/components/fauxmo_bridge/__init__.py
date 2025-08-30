# components/fauxmo_bridge/__init__.py
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID, CONF_NAME

DEPENDENCIES = ["wifi"]  # fauxmo needs networking up

fauxmo_bridge_ns = cg.esphome_ns.namespace("fauxmo_bridge")
FauxmoBridgeComponent = fauxmo_bridge_ns.class_("FauxmoBridgeComponent", cg.Component)

CONF_MDNS_NAME = "mdns_name"
CONF_TCP_PORT = "tcp_port"
CONF_LIGHTS = "lights"
CONF_LIGHT_ID = "light_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(FauxmoBridgeComponent),
    cv.Optional(CONF_MDNS_NAME, default=""): cv.string,
    cv.Optional(CONF_TCP_PORT, default=80): cv.port,  # Alexa Gen3 prefers 80
    cv.Optional(CONF_LIGHTS, default=[]): cv.ensure_list(cv.Schema({
        cv.Required(CONF_NAME): cv.string_strict,
        cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState),
    })),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    # Pull your library straight from GitHub
    cg.add_library(
        name="fauxmoESP",
        repository="https://github.com/klaudyu/fauxmoESP.git#v4-esphome",
        version=""
    )

    # (Optional but recommended) use ESPHome-maintained Async libs for fewer toolchain issues
    # ESP32:
    cg.add_library("esphome/AsyncTCP-esphome", None)
    # If your fauxmo build path uses Async WebServer features, also include:
    # cg.add_library("esphome/ESPAsyncWebServer-esphome", None)
    # ESP8266 alternative (only if you target esp8266):
    # cg.add_library("esphome/ESPAsyncTCP-esphome", None)

    if config[CONF_MDNS_NAME]:
        cg.add(var.set_mdns_name(config[CONF_MDNS_NAME]))
    if CONF_TCP_PORT in config:
        cg.add(var.set_tcp_port(config[CONF_TCP_PORT]))

    for lconf in config[CONF_LIGHTS]:
        ls = await cg.get_variable(lconf[CONF_LIGHT_ID])
        cg.add(var.add_light(lconf[CONF_NAME], ls))

    await cg.register_component(var, config)
