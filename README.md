# ESPHome Fauxmo Bridge

Expose ESPHome lights to Alexa via **fauxmoESP**, no cloud needed.

## Install

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/klaudyu/fauxmoESP-ESPhomeBridge
      ref: main
    components: [fauxmo_bridge]
