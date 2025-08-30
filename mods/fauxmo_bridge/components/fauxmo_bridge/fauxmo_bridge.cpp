#include "fauxmo_bridge.h"
#include <cmath>
#include <algorithm>

namespace esphome {
namespace fauxmo_bridge {

static const char *const TAG = "fauxmo_bridge";

void FauxmoBridgeComponent::setup() {
  ESP_LOGI(TAG, "fauxmo bridge will start after WiFi connects (port=%u, mdns='%s')",
           (unsigned)tcp_port_, mdns_name_.c_str());
  // Do NOT start fauxmo here.
}

void FauxmoBridgeComponent::loop() {
  // Bring up fauxmo only once Wi-Fi is actually up and we have an IP
  if (!started_ && WiFi.status() == WL_CONNECTED && WiFi.localIP() != INADDR_NONE) {
    start_fauxmo_();
  }
  if (started_) {
    fauxmo_.handle();
  }
}



void FauxmoBridgeComponent::start_fauxmo_() {
  ESP_LOGI(TAG, "Starting fauxmo bridge…");
  fauxmo_.setPort(80);
  fauxmo_.createServer(true);
  if (tcp_port_ != 0) fauxmo_.setPort(tcp_port_);
  if (!mdns_name_.empty()) fauxmo_.enableMDNS(mdns_name_.c_str());

	// RE-ENABLE fauxmo's own mDNS
	if (!mdns_name_.empty()) {
	  fauxmo_.enableMDNS(mdns_name_.c_str());
	}

  // (re)attach the onSetState handler
  fauxmo_.onSetState([this](unsigned char idx, const fauxmoesp_device_t *dev) {
    if (idx >= lights_.size()) return;
    auto *ls = lights_[idx].state;
    if (!ls) return;
    auto call = ls->make_call();
    call.set_state(dev->state);
    call.set_brightness(float(dev->value) / 254.0f);
    if (strncmp(dev->colormode, "hs", 2) == 0) {
      float hue_deg = (float(dev->hue) / 65535.0f) * 360.0f;
      float sat = float(dev->saturation) / 255.0f;
      float r,g,b; hsv_to_rgb(hue_deg, sat, 1.0f, r, g, b);
      call.set_rgb(r,g,b);
    } else if (strncmp(dev->colormode, "xy", 2) == 0) {
      float r,g,b; xy_to_rgb(dev->x, dev->y, (float)dev->value / 254.0f, r, g, b);
      call.set_rgb(r,g,b);
    } else if (strncmp(dev->colormode, "ct", 2) == 0 && dev->ct > 0) {
      uint32_t kelvin = (uint32_t)(1000000UL / (uint32_t)dev->ct);
      call.set_color_temperature(kelvin);
    }
    call.perform();
  });

  delay(150);
  fauxmo_.enable(true);
  ready_ = true;
  started_ = true;
  ESP_LOGI(TAG, "fauxmo bridge ready: devices=%u", (unsigned)lights_.size());
}

void FauxmoBridgeComponent::stop_fauxmo_() {
  if (!started_) return;
  fauxmo_.enable(false);
  ready_ = false;
  started_ = false;
  ESP_LOGI(TAG, "fauxmo bridge stopped");
}

void FauxmoBridgeComponent::add_light(const std::string &name, LightState *state) {
  LightEntry e;
  e.state = state;
  e.fauxmo_id = fauxmo_.addDevice(name.c_str());
  lights_.push_back(e);

  // ESPHome → Hue: keep fauxmo side updated when local state changes
  // ESPHome → Hue: update fauxmo when the light reaches its target state
  unsigned char faux_id = e.fauxmo_id;              // avoid C++14 capture-initializer
  state->add_new_target_state_reached_callback([this, state, faux_id]() {
    float b = 0.0f;
    state->current_values_as_brightness(&b);
    const uint8_t bri = static_cast<uint8_t>(std::round(b * 255.0f));
    this->fauxmo_.setState(faux_id, (bri > 0), bri);
    this->fauxmo_.notifyState(faux_id);
  });

  ESP_LOGI(TAG, "Added light '%s' (fauxmo id=%u)", name.c_str(), (unsigned)e.fauxmo_id);
}

// --- small color helpers ---
void FauxmoBridgeComponent::hsv_to_rgb(float h_deg, float s, float v, float &r, float &g, float &b) {
  float c = v * s;
  float x = c * (1 - fabsf(fmodf(h_deg / 60.0f, 2.0f) - 1));
  float m = v - c;
  float rr, gg, bb;
  int hi = int(h_deg / 60.0f) % 6;
  switch (hi) {
    case 0: rr=c; gg=x; bb=0; break;
    case 1: rr=x; gg=c; bb=0; break;
    case 2: rr=0; gg=c; bb=x; break;
    case 3: rr=0; gg=x; bb=c; break;
    case 4: rr=x; gg=0; bb=c; break;
    default: rr=c; gg=0; bb=x; break;
  }
  r = rr + m; g = gg + m; b = bb + m;
}

void FauxmoBridgeComponent::xy_to_rgb(float x, float y, float bri, float &r, float &g, float &b) {
  if (y <= 0.0f) y = 1e-7f;
  float z = 1.0f - x - y;
  float Y = bri;
  float X = (Y / y) * x;
  float Z = (Y / y) * z;

  float rr =  1.656492f*X - 0.354851f*Y - 0.255038f*Z;
  float gg = -0.707196f*X + 1.655397f*Y + 0.036152f*Z;
  float bb =  0.051713f*X - 0.121364f*Y + 1.011530f*Z;

  auto gamma = [](float u){ return (u <= 0.0031308f) ? 12.92f*u : 1.055f*powf(u, 1.0f/2.4f) - 0.055f; };
  rr = gamma(std::max(0.0f, rr));
  gg = gamma(std::max(0.0f, gg));
  bb = gamma(std::max(0.0f, bb));

  float maxc = std::max(rr, std::max(gg, bb));
  if (maxc > 1.0f) { rr/=maxc; gg/=maxc; bb/=maxc; }

  r = rr; g = gg; b = bb;
}

}  // namespace fauxmo_bridge
}  // namespace esphome
