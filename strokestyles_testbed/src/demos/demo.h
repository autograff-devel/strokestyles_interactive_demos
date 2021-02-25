#pragma once
#include "colormotor.h"
#include "common.h"

using namespace cm;
using namespace ag;

class Demo : public AppModule {
 public:
  ParamList gui_params;

  Demo(const std::string& name) : AppModule(name) {}
  virtual ~Demo() {
    if (ui_file_watcher) {
      ui_file_watcher->stop();
      delete ui_file_watcher;
    }
  }
  SimpleJsonUI simple_ui;
  json         ui_json;
  bool         use_simple_ui   = true;
  FileWatcher* ui_file_watcher = 0;

  void load_simple_ui() {
    // simple UI setup
    std::cout << "Loading simple UI for " << this->filename() << std::endl;
    reload_ui_json();
    simple_ui.build(&gui_params);
    simple_ui.print_all_params();
  }

  std::string simple_ui_path() {
    return default_configuration_path(this, "_simple_ui.json");
  }

  void reload_ui_json() {
    std::cout << "Reloading " << simple_ui_path() << std::endl;

    if (!ag::file_exists(simple_ui_path())) {
      ag::string_to_file("[\n\t{\"Parameters\":[]}]\n", simple_ui_path());
    }
    std::string jsonstr = string_from_file(simple_ui_path());

    try {
      simple_ui.print_all_params();
      ui_json = json::parse(jsonstr);
    } catch (json::parse_error& e) {
      std::cout << "Error parsing simple ui\n";
      std::cout << e.what() << '\n';
      ui_json       = json::parse("{}");
      use_simple_ui = false;
    }
  }

  void load_parameters() {
    // if (!gui_params.loadXml(configuration_path(this, ".xml")))
    if (!file_exists(configuration_path(this, ".xml")))
      load_defaults();
    else
      load_params_request(configuration_path(this, ".xml"));
  }

  void save_parameters() {
    save_params_request(configuration_path(this, ".xml"));
  }

  virtual bool save_params_request(const std::string& path) {
    gui_params.saveXml(path);
    return true;
  }

  virtual void load_params_request(const std::string& path) {
    gui_params.loadXml(path);
  }

  void* getData() { return &gui_params; }

  virtual void load_defaults() {
    gui_params.loadXml(default_configuration_path(this, "_default.xml"));
  }

  void parameter_settings_gui() {
    static bool dont_ask_me_next_time = false;
    bool        save_defaults         = false;
    bool        watching              = (ui_file_watcher != 0);
    bool        open_defaults_popup   = false;

    if (ImGui::Button("+ Parameter Settings..."))
      ImGui::OpenPopup("param_settings_popup");
    if (ImGui::BeginPopup("param_settings_popup")) {
      if (ImGui::MenuItem("Reload Simple UI")) reload_ui_json();

      if (ImGui::BeginMenu("UI settings path:")) {
        ImGui::MenuItem(simple_ui_path().c_str());
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem("Open UI settings file")) {
        std::string cmd = "open ";
        cmd += simple_ui_path();
        system(cmd.c_str());
      }
      if (ImGui::MenuItem("Watch UI file", "", &watching)) {
        if (!watching && ui_file_watcher) {
          ui_file_watcher->stop();
          delete ui_file_watcher;
          ui_file_watcher = 0;
        } else if (watching && !ui_file_watcher) {
          ui_file_watcher = new FileWatcher(simple_ui_path());
        }
      }
      ImGui::MenuItem("(Does not work with Xcode (atomic))",
                      NULL,
                      false,
                      false);

      if (ImGui::MenuItem("Reset Default Parameters")) {
        load_defaults();
      }
      if (ImGui::MenuItem("Save Defaults")) {
        if (dont_ask_me_next_time)
          save_defaults = true;
        else
          open_defaults_popup = true;
        // ImGui::OpenPopup("Save Defaults?");
      }
      ImGui::EndPopup();
    }

    // if (ImGui::BeginMenu("Parameter setting")) {
    //   if (ImGui::MenuItem("Reload Simple UI")) reload_ui_json();
    //   if (ImGui::BeginMenu("UI settings path:")) {
    //     ImGui::MenuItem(simple_ui_path().c_str());
    //     ImGui::EndMenu();
    //   }
    //   if (ImGui::MenuItem("Open UI settings file")) {
    //     std::string cmd = "open ";
    //     cmd += simple_ui_path();
    //     system(cmd.c_str());
    //   }
    //   if (ImGui::MenuItem("Watch UI file", "", &watching)) {
    //     if (!watching && ui_file_watcher) {
    //       ui_file_watcher->stop();
    //       delete ui_file_watcher;
    //       ui_file_watcher = 0;
    //     } else if (watching && !ui_file_watcher) {
    //       ui_file_watcher = new FileWatcher(simple_ui_path());
    //     }
    //   }
    //   ImGui::MenuItem("(Does not work with Xcode (atomic))",
    //                   NULL,
    //                   false,
    //                   false);

    //   if (ImGui::MenuItem("Reset Default Parameters")) {
    //     load_defaults();
    //   }
    //   if (ImGui::MenuItem("Save Defaults")) {
    //     if (dont_ask_me_next_time)
    //       save_defaults = true;
    //     else
    //       open_defaults_popup = true;
    //     // ImGui::OpenPopup("Save Defaults?");
    //   }

    //   ImGui::EndMenu();
    // }

    if (ui_file_watcher && ui_file_watcher->hasFileChanged()) {
      reload_ui_json();
    }

    if (open_defaults_popup) ImGui::OpenPopup("Save Defaults?");

    if (ImGui::BeginPopupModal("Save Defaults?",
                               NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text(
          "Are you sure you want to overwrite the default parameter settings?");
      ImGui::Separator();

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
      ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
      ImGui::PopStyleVar();

      if (ImGui::Button("OK", ImVec2(120, 0))) {
        save_defaults = true;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SetItemDefaultFocus();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        save_defaults = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (save_defaults) {
      std::cout << "Saving default paramters\n";
      gui_params.saveXml(default_configuration_path(this, "_default.xml"));
    }
  }

  void simple_or_normal_gui() {
    if (ImGui::BeginTabBar("UiTabBar", ImGuiTabBarFlags_None)) {
      ///
      if (use_simple_ui && ImGui::BeginTabItem("Simple UI")) {
        ImGui::BeginChild("SimpleParamUI");
        simple_ui.ui(ui_json);
        ImGui::EndChild();

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("All params")) {
        ImGui::BeginChild("ParamUI");
        imgui(gui_params);
        ImGui::EndChild();

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }

  void parameter_gui() {
    parameter_settings_gui();
    simple_or_normal_gui();
  }
};
