#pragma once
#include "cm.h"

namespace cm {
float  appWidth();
float  appHeight();
double appFrameTime();
V2     appCenter();

class AppModule {
 public:
  /// The name will show up in the GUI eventually
  AppModule(const std::string name) : name(name), active(true) {}

  /// Initialize
  virtual bool init() { return false; }
  virtual bool gui() { return false; }
  virtual void menu() {}
  virtual void update() {}
  virtual void render() {}
  virtual void exit() {}

  virtual void activated() {}
  virtual void deactivated() {}

  virtual bool save_params_request(const std::string& path) { return false; }
  virtual void load_params_request(const std::string& path) {}

  virtual void* getData() { return 0; }

  std::string filename() const {
    std::string fname = name;
    std::replace(fname.begin(), fname.end(), ' ', '_');
    return fname;
  }

  std::string name;
  bool        active;
};

}  // namespace cm
