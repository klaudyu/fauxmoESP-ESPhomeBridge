#pragma once
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_call.h"

#include "fauxmoESP.h"  // your library



#include <WiFi.h>  // add this




namespace esphome {
namespace fauxmo_bridge {

using esphome::light::LightCall;
using esphome::light::LightState;

class FauxmoBridgeComponent : public Component {
 public:
  void set_mdns_name(const std::string &s) { this->mdns_name_ = s; }
  void set_tcp_port(uint16_t p) { this->tcp_port_ = p; }

  void add_light(const std::string &name, LightState *state);

  void setup() override;
  void loop() override;
  

  void start_fauxmo_();
  void stop_fauxmo_();
  bool started_{false};

 protected:
  struct LightEntry {
    LightState *state{nullptr};
    unsigned char fauxmo_id{0};
  };

  // helpers
  static void hsv_to_rgb(float h_deg, float s, float v, float &r, float &g, float &b);
  static void xy_to_rgb(float x, float y, float bri, float &r, float &g, float &b);

  fauxmoESP fauxmo_;
  std::vector<LightEntry> lights_;
  std::string mdns_name_;
  uint16_t tcp_port_{1905};
  bool ready_{false};
};

}  // namespace fauxmo_bridge
}  // namespace esphome
