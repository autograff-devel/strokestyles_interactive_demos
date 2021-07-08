/// Main app
/// Note: it should not be necessary to modify much here
/// except for adding your AppModule's
/// or changing the window size/name
/// Unless some more complex logic has to happen.

#include "cm_imgui_app.h"
#include "colormotor.h"
#include "demos/include_modules.h"
// Add temporary prototypes to the src/prototypes directory
// following is automatically defined by cmake if directory exists
#ifdef INCLUDE_PROTOTYPES
#include "strokestyles_prototypes/include_modules.h"
#endif

#ifdef USE_SYPHON
#include "syphon_wrapper/syphon_wrapper.h"

static bool           syphon_active = false;
static syphon::Server syphon_server;
static Texture*       syphon_texture;
#endif

GlyphDatabase FontStylizationBase::db;
std::string   FontStylizationBase::glyph_dir = "data/glyph_data/glyphs";

void FontStylizationBase::load_global_settings() {
  if (!file_exists("global_settings.json")) return;
  json js                        = ag::json_from_file("global_settings.json");
  FontStylizationBase::glyph_dir = relative_path(js["glyph_dir"]);
  int w                          = js.value("window_w", -1);
  int h                          = js.value("window_h", -1);
  if (w > 0 && h > 0)
    setWindowSize(w, h);
  int x = js.value("window_x", -1);
  int y = js.value("window_y", -1);
  if (x > 0 && x > 0)
    setWindowPos(x, y);
}

void FontStylizationBase::save_global_settings() {
  json js;
  js["glyph_dir"] = relative_path(FontStylizationBase::glyph_dir);
  int x, y, w, h;
  getWindowSize(&w, &h);
  getWindowPos(&x, &y);
  js["window_w"] = w;
  js["window_h"] = h;
  js["window_x"] = x;
  js["window_y"] = y;

  json_to_file(js, "global_settings.json", true);
}

using namespace cm;
using namespace cv;
using namespace cv::ogl;

static std::vector<AppModule*> modules;
static int                     curModule        = 0;
static bool                    save_eps         = false;
static int                     max_video_frames = 600;
static DemoPresetLoader        demopresets;
static ParamList               global_dbg_params;

namespace cm {
class IconButtons {
 public:
  IconButtons() { font = 0; }

  void load(const std::string& path, float size = 32) {
    icon_size   = size;
    ImGuiIO& io = ImGui::GetIO();
    font        = io.Fonts->AddFontFromFileTTF(path.c_str(), icon_size);
  }

  ~IconButtons() {}

  bool button(const std::string& name, char c) {
    ImGui::PushFont(font);
    std::stringstream ss;
    ss << c;
    ImGui::PushID(name.c_str());
    ImVec2 size(0, 0);
    size.x   = icon_size;
    bool res = ImGui::Button(ss.str().c_str(), size);
    ImGui::PopID();
    ImGui::PopFont();
    return res;
  }

  bool selectable(const std::string& name, char c, bool state) {
    ImGui::PushFont(font);
    std::stringstream ss;
    ss << c;
    ImGui::PushID(name.c_str());
    ImVec2 size(0, 0);
    size.x   = icon_size;
    bool res = ImGui::Selectable(ss.str().c_str(), state, 0, size);
    ImGui::PopID();
    ImGui::PopFont();
    return res;
  }

  bool selectable(const std::string& name,
                  const std::string& items,
                  int*               selection) {
    ImGui::PushFont(font);

    int  sel = *selection;
    bool res = false;
    ImGui::Text(name.c_str());
    ImGui::SameLine();

    for (int i = 0; i < items.length(); i++) {
      std::stringstream ss;
      ss << name;
      ss << items[i];
      ImVec2 size(0, 0);
      size.x = icon_size;
      if (ImGui::Selectable(ss.str().c_str(), i == *selection, 0, size)) {
        sel = i;
        res = true;
      }
      if (i < items.length() - 1) ImGui::SameLine();
    }

    *selection = sel;
    ImGui::PopFont();

    return res;
  }

  ImFont* font;
  float   icon_size;
};

namespace lines {
cm::Image image;
float     width;

Buffer buffer;

bool init(const std::string& path, int n) {
  image  = Image(path);
  buffer = Buffer(n);
  return true;
}

void release() {
  image.release();
}

void begin(double w) {
  width = w;
  image.bind();
  buffer.clear();
}

void segment(const arma::vec& a, const arma::vec& b) {
  buffer.segment(a, b, width);
}

void draw(const ag::Polyline& P) {
  buffer.polyline(P, width);
}

void draw(const Buffer& buf) {
  image.bind();
  cm::gfx::draw(buf.mesh);
  image.unbind();
}

void end() {
  cm::gfx::draw(buffer.mesh);
  image.unbind();
}

}  // namespace lines
//

void similarity_preload_hack() {
  std::string demoname = "Similarity Font Stylization";
  AppModule*  module   = 0;
  for (int i = 0; i < modules.size(); i++) {
    if (modules[i]->name == demoname) {
      module = modules[i];
    }
  }

  if (!module)
    return;

  for (const DemoPresetLoader::Entry& e : demopresets.entries) {
    if (e.mode_name == "Similarity Font Stylization") {
      module->load_params_request(
          demopresets.preset_path(e.name));
    }
  }
}

int appInit(void* userData, int argc, char** argv) {
  // std::stringstream dirstr;
  // dirstr << getExecutablePath() << "/..";
  // chdir(dirstr.str().c_str());

#include "demos/instantiate_modules.incl"

#ifdef INCLUDE_PROTOTYPES
#include "strokestyles_prototypes/instantiate_modules.incl"
#endif

  lines::init("data/lineim.png");

  // initialize global debug params
  global_dbg_params.addBool("debug draw", &global_debug.debug_draw)
      ->describe("Toggle mode-specific debug visualization");
  global_dbg_params.addBool("draw gaussians", &global_debug.draw_gaussians)
      ->describe("Toggle drawing Gaussians for smoothing");
  global_dbg_params.addFloat("sigma mul", &global_debug.sigma_mul, 0.01, 2.)
      ->describe("Gaussian covariance scale factor");
  global_dbg_params.addBool("draw partitions", &global_debug.draw_partitions)
      ->describe("Draw partition shapes for layering");
  global_dbg_params.addBool("draw normals", &global_debug.draw_normals)
      ->describe("Draw normals for trajectories");
  global_dbg_params.addBool("Log output", &global_debug.dump_output)
      ->describe("Debug log (mode dependent)");
  global_dbg_params.addBool("Fancy outlines", &global_debug.fancy_outlines)
      ->describe("Draw fancy outlines");
  global_dbg_params.addBool("Debug Layering", &global_debug.debug_layering);
  global_dbg_params.addBool("Debug Folds", &global_debug.debug_folds);
  global_dbg_params.addBool("Debug List Graph", &global_debug.debug_list_graph);
  global_dbg_params.addInt("Magic index", &global_debug.magic_index);
  global_dbg_params.loadXml("global_dbg.xml");
  // initialize
  for (int i = 0; i < modules.size(); i++) modules[i]->init();

  ui::init();

  demopresets.read_json();
  FontStylizationBase::load_global_settings();
  FontStylizationBase::db.load(FontStylizationBase::glyph_dir);

  similarity_preload_hack();

#ifdef USE_SYPHON
  syphon_texture = new Texture(fbWidth(), fbHeight(), Texture::A8R8G8B8);
#endif
  return 1;
}

void select_preset(bool menu) {
  int select = demopresets.gui(modules[curModule], menu);
  if (select != -1) {
    std::cout << "Selecting " << select << std::endl;
    for (int i = 0; i < modules.size(); i++)
      if (modules[i]->name == demopresets.entries[select].mode_name) {
        curModule = i;
        std::cout << "Mode: " << i << std::endl;
        modules[curModule]->load_params_request(
            demopresets.current_preset_path());
        modules[curModule]->activated();
        break;
      }
  }
}

void appGui() {
  ImGui::BeginMainMenuBar();

  if (ImGui::BeginMenu("Demo selection <-|")) {
    for (int i = 0; i < modules.size(); i++) {
      bool active = (i == curModule);
      if (ImGui::MenuItem(modules[i]->name.c_str(), NULL, &active)) {
        curModule = i;
        // osc.set_params((ParamList*)modules[i]->getData());
      }
    }

    ImGui::EndMenu();
  }

  modules[curModule]->menu();
  std::stringstream ss;
  ss << " " << modules[curModule]->name.c_str() << " - FPS: "
     << (int)ImGui::GetIO()
            .Framerate;  // << " exec path: " << getExecutablePath();
  ImGui::BeginMenu(ss.str().c_str(), false);

  if (ImGui::BeginMenu("Options")) {
    if (ImGui::MenuItem("Set Glyph Database...")) {
      std::string path;
      if (openFolderDialog(path, "Select glyph database folder")) {
        FontStylizationBase::glyph_dir = relative_path(path);
        // gui_params.setString("glyph dir", glyph_dir);
        FontStylizationBase::db.load(FontStylizationBase::glyph_dir);
        FontStylizationBase::save_global_settings();
      }
    }

    if (ImGui::MenuItem("Save Screen as SVG...")) {
      save_eps = true;
    }
    ImGui::EndMenu();
  }

  select_preset(true);
  ImGui::EndMainMenuBar();

  ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);

  ImGui::Begin(modules[curModule]->name.c_str());

  if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_None)) {
    ///
    if (ImGui::BeginTabItem("Demo")) {
      ImGui::BeginChild(
          "DemoChild");  //, ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f,
                         // 260), false, window_flags);

      modules[curModule]->gui();
      ImGui::EndChild();

      ImGui::EndTabItem();
    }
    ///
    if (ImGui::BeginTabItem("Presets")) {
      select_preset(false);
      ImGui::EndTabItem();
    }
    ///
    if (ImGui::BeginTabItem("Video recording")) {
#ifdef USE_SYPHON
      ImGui::Checkbox("Syphon active", &syphon_active);
#endif

      ImGui::InputInt("max video frames", &max_video_frames);

      {
        const char*               items[]      = {"Select",
                               "800x800",
                               "1024x768",
                               "1280×720",
                               "1920x1080"};
        const std::pair<int, int> sizes[]      = {{800, 800},
                                             {1024, 768},
                                             {1280, 720},
                                             {1920, 1080}};
        static int                item_current = 0;
        if (ImGui::Combo("Set Window Size",
                         &item_current,
                         items,
                         IM_ARRAYSIZE(items))) {
          if (item_current != 0) {
            setWindowSize(sizes[item_current - 1].first,
                          sizes[item_current - 1].second);
          }
          item_current = 0;
        }
      }

      if (ImGui::Button("Save Video...")) {
        std::string path;
        if (saveFileDialog(path, "mp4")) {
          gfx::beginScreenRecording(path, appWidth(), appHeight(), 30.);
          // osc.start_recording();
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop recording")) gfx::endScreenRecording();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Debug Settings")) {
      imgui(global_dbg_params);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  // ImGui::SetNextWindowPos(ImVec2(60, 300), ImGuiCond_FirstUseEver);
  // ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
  // ImGui::Begin("DemoPresetLoader", 0);

  // ImGui::End();

  //ImGui::ShowDemoWindow();
  ImGui::End();
}

void appRender(float w, float h) {
  modules[curModule]->update();

  gfx::clear(0.2, 0.2, 0.2, 1.0);

  gfx::enableDepthBuffer(false);
  gfx::setOrtho(w, h);
  gfx::setBlendMode(gfx::BLENDMODE_ALPHA);
  if (save_eps) {
    std::string path;
    if (saveFileDialog(path, "svg")) {
      gfx::beginEps(path, Box(0, 0, appWidth(), appHeight()));
    } else {
      save_eps = false;
    }
  }

  modules[curModule]->render();
//CM_GLERROR;
#ifdef USE_SYPHON
  if (syphon_active) {
    if (fbWidth() != syphon_texture->getWidth() || fbHeight() != syphon_texture->getHeight()) {
      delete syphon_texture;
      syphon_texture = new Texture(fbWidth(), fbHeight(), Texture::A8R8G8B8);
    }
    syphon_texture->grabFrameBuffer();
    syphon_server.publish(syphon_texture->getId(),
                          GL_TEXTURE_2D,
                          syphon_texture->getWidth(),
                          syphon_texture->getHeight(),
                          false);
  }
#endif

  gfx::saveScreenFrame();

  if (save_eps) {
    gfx::endEps();
    save_eps = false;
  }
}

void appExit() {
  FontStylizationBase::save_global_settings();
  for (int i = 0; i < modules.size(); i++) {
    modules[i]->exit();
    delete modules[i];
  }
  global_dbg_params.saveXml("global_dbg.xml");
  modules.clear();
  lines::release();

#ifdef USE_SYPHON
  syphon_server.release();
  delete syphon_texture;
#endif
}

}  // namespace cm

int main(int argc, char** argv) {
  imguiApp(
      argc,
      argv,
      "strokestyles testbed",
      1280,
      720,
      &appInit,
      &appExit,
      &appGui,
      &appRender,
      "data/NotoSansCJKtc-Bold.ttf",  // (getExecutablePath() +
                                      // "data/NotoSansCJKtc-Bold.ttf").c_str(),
      14,
      cm::GLYPH_SUPPORT_KOREAN | cm::GLYPH_SUPPORT_JAPANESE);
}
