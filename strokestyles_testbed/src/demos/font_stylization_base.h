#pragma once
#include <boost/filesystem.hpp>

#include "GLFW/glfw3.h"
#include "ag_alpha_shape.h"
#include "ag_color.h"
#include "ag_graff_font.h"
#include "ag_layering.h"
#include "ag_schematize.h"
#include "ag_skeletal_strokes.h"
#include "ag_svg.h"
#include "autograff.h"
#include "demo.h"
#include "deps/json.hpp"
#include "deps/tinyutf8.h"
#include "deps/utf8.h"

using json = nlohmann::json;

using namespace cm;
using namespace ag;
using namespace arma;

class FontStylizationBase : public Demo {
 public:
  int         tool = 0;
  GlyphEditor editor;

  struct {
    float seed       = 10.;
    bool  frame_seed = true;  // sets random seed every frame
    float w          = 40.;
    float anim_time  = 1;

    bool  debug_draw          = false;
    bool  rt_update           = true;
    bool  transparent_save    = true;
    bool  draw_stroke_polygon = false;
    bool  draw_original       = false;
    float original_offset     = 1;

    float angle_speed         = 0;
    bool  animate_start_angle = false;

    std::string text          = "R";
    bool        show_original = true;
    int         font_index    = 0;
    std::string font_name     = "";

    float chunk_line_width = 2.;

    bool show_serifs = false;

    // float tx        = 0;
    // float ty        = 0;
    float spacing = 1.;
    // float font_size = 200;

    bool gui_active = true;

    // float min_merge_length = 50; DISABLE

    bool easy_ui = false;

    int                      schematization_preset  = 0;
    std::vector<std::string> schematization_presets = {
        "None", "20°", "30°", "36°", "45°", "60°", "90°"};
    std::vector<int> schematization_C = {0, 9, 6, 5, 4, 3, 2};
    // from C to
    std::map<int, int> C_to_preset = {
        {0, 0}, {9, 1}, {6, 2}, {5, 3}, {4, 4}, {3, 5}, {2, 6}};
  } params;

  struct {
    float x        = 0;
    float y        = 0;
    float zoom     = 1;
    float rotation = 0;
  } camera;

  struct {
    V4 color = V4(0.9, 0.9, 0.9, 1.);
  } background_params;

  StrokeRenderData render_data;
  SkeletalParams   skel_params;
  PaletteManager   palette;

  struct {
    std::vector<std::string> files;
    std::vector<std::string> names;
    int                      current = -1;
  } presets;

  cm::ParamModifiedTracker  param_modified;
  cm::ParamModifiedTracker  simplify_modified;
  bool                      font_modified = false;
  std::vector<PolylineList> outline;
  std::vector<GlyphSerifs>  serifs;

  Trigger<bool> trig_save_png;

  GlyphStringParams string_params;
  PolylineList original_shape;

  std::vector<StylizedGlyph> glyphs;
  std::vector<GlyphInstance> str;
  aa_box                     string_bbox;
  bool                       view_transform = true;  // <- If true, mode_render is transformed
  mat                        view, centerm;
  double                     view_scale = 1;  // Scale factor for viewing transform

  bool is_stroke_based = true;  // <- If false functionalities such as schematization
                         // are not required
  bool shift_original = true;

  virtual void mode_gui() = 0;
  virtual void mode_menu() {}
  virtual void mode_render(bool needs_update) = 0;
  virtual void mode_init() {}
  virtual void mode_exit() {}

  static GlyphDatabase db;
  static std::string   glyph_dir;

  static void load_global_settings();
  static void save_global_settings();

  std::set<std::string> jap_fonts;

  std::string preset_dir;

  FontStylizationBase(const std::string& name            = "Basic Font Stylization",
                      const std::string& preset_dir      = "./data/presets",
                      bool               is_stroke_based = true)
      : Demo(name), palette("./data/palettes"), preset_dir(preset_dir) {
    ParamList* child;
    param_modified << gui_params.addFloat("random seed", &params.seed, 1, 1000)
                          ->describe("random seed");

    string_params.simplify_eps = 0.01;

    // param_modified << gui_params.addString("glyph dir", &glyph_dir)->noGui();

    // clang-format off
    // gui_params.addBool("debug draw", &params.debug_draw)->appendOption("n");
    gui_params.addBool("draw stroke polygon", &params.draw_stroke_polygon);
    gui_params.addBool("draw original", &params.draw_original);
		gui_params.addFloat("original offset", &params.original_offset, 0., 1.);
		gui_params.addBool("shift original", &shift_original);

    gui_params.addBool("transparent save", &params.transparent_save)->noGui();
    gui_params.addBool("update every frame", &params.rt_update);
		gui_params.addFloat("outline thickness", &params.chunk_line_width, 0.5, 5.);

	if (is_stroke_based)
	{
		param_modified << gui_params.addBool("g_use_ref_width_adjustment", &g_use_ref_width_adjustment);
		param_modified << gui_params.addBool("pre_rectify_load", &global_debug.pre_rectify_load);
		param_modified << gui_params.addBool("pre_rectify_schematize", &global_debug.pre_rectify_schematize);
		//gui_params.addFloat("sigma mul", &render_data.softie_params.sigma_mul, 0.01, 2.);
		gui_params.addFloat("anim time", &params.anim_time, 0, 1);
	}

    // param_modified << gui_params.addBool("phunk",
    // &params.phunk)->describe("turns on ze phunk");
    // param_modified << gui_params
    //                       .addFloat("min_merge_length",
    //                                 &params.min_merge_length,
    //                                 5,
    //                                 300)
    //                       ->describe("Minimum length to perform merge with");
    //

	if (is_stroke_based)
	{
		simplify_modified << gui_params.addFloat("simplify eps", &string_params.simplify_eps, 0.00, 10)
			->describe("Imput stroke simplification tolerance");
		simplify_modified << gui_params.addFloat("straighten thresh", &string_params.straighten_thresh, 0.0, 4)
			->describe("Straightening tolerance");
		gui_params.addBool("gui active", &params.gui_active);
	}
	
    gui_params.newChild("Camera");
    gui_params.addFloat("pos x", &camera.x, -1300, 1300);
    gui_params.addFloat("pos y", &camera.y, -1300, 1300);
    gui_params.addFloat("zoom", &camera.zoom, 0.1, 2);
    gui_params.addFloat("rotation", &camera.rotation, 0, 360);

    gui_params.newChild("Text");
    param_modified << gui_params.addString("text", &params.text)->describe("Input text");

    //param_modified << gui_params.addFloat("tx", &params.tx, -300, 300);
    //param_modified << gui_params.addFloat("ty", &params.ty, -600, 600);
    param_modified << gui_params.addFloat("spacing", &string_params.spacing, 0.1, 2.)
			->describe("Spacing between letters");
    // param_modified << gui_params.addFloat("font size", &string_params.size,
    // 100, 1000);
    //gui_params.addInt("font index", &params.font_index)->noGui();
    param_modified << gui_params.addString("font name", &params.font_name)->noGui();

	if (is_stroke_based)
	{
		gui_params.newChild("Schematization");  //->appendOption("hidden");
		param_modified << gui_params.addInt("C", &string_params.C)
			->describe("Angle quantization (180/C)");
		// This was dodgy
		// param_modified << gui_params.addSelection("Quantization angle",
		// params.schematization_presetsi,
		// &params.schematization_preset)->describe("Quantization angle for
		// schematization");
		param_modified << gui_params.addFloat("start angle", &string_params.start_ang, 0, 360)
			->describe("Initial angle for schematization");
		// param_modified << gui_params.addFloat("interpolation lowpass",
		// &string_params.interpolation_lowpass, 0., 1.0);
		param_modified << gui_params.addBool("also opposite angle", &string_params.schematize_also_opposite_angle);
		param_modified << gui_params.addBool("adjust struct", &string_params.adjust_struct);
		param_modified << gui_params.addBool("rectify struct", &string_params.rectify);
		// param_modified << gui_params.addBool("animate start angle",
		// &params.animate_start_angle);
		param_modified << gui_params.addFloat("angle speed", &params.angle_speed, 0., 1.);

		gui_params.newChild("Structural modifiers");
		param_modified << gui_params.addFloat("free end extension", &string_params.free_end_extension, -2, 4.0)
			->describe("Extension for terminals");
		param_modified << gui_params.addFloat("extension x", &string_params.extension_x, 0.0, 1.0)
			->describe("x axis extension amount");
		param_modified << gui_params.addFloat("extension y", &string_params.extension_y, 0.0, 1.0)
			->describe("y axis extension amount");

		param_modified << gui_params.addFloat("wiggle_length_thresh", &string_params.length_thresh, 0., 100.0);
		param_modified << gui_params.addFloat("horizontal_wiggle_thresh", &string_params.horizontal_wiggle_thresh, -2.0, 2.0);
		param_modified << gui_params.addFloat("vertical_wiggle_thresh", &string_params.vertical_wiggle_thresh, -2.0, 2.0);

		param_modified << gui_params.addBool("show serifs", &params.show_serifs);
        param_modified << gui_params.addBool("stroke serifs", &string_params.stroked_serifs);
		param_modified << gui_params.addFloat("serif thresh", &string_params.serif_thresh, 0.0, 20.)
			->describe("< ratio between serif length and incident stroke width"); // ratio between end width
		param_modified << gui_params.addBool("convert serifs", &string_params.convert_serifs);
		param_modified << gui_params.addFloat("serif probability", &string_params.serif_proba, 0, 1.);
		// param_modified << gui_params.addFloat("wiggle_length",
		// &string_params.wiggle_length, 0.1, 3.0); param_modified <<
		// gui_params.addFloat("wiggle_width", &string_params.wiggle_width,
		// -3., 3.0); param_modified << gui_params.addFloat("wiggle_cross",
		// &string_params.wiggle_cross, 0.1, 3); param_modified <<
		gui_params.addFloat("wiggle_radius", &string_params.wiggle_radius, 0.001, 200);
  
  //  param_modified << gui_params.addFloat("wiggle_initial_thickness",
		// &string_params.wiggle_initial_thickness, 0.05, 2.);

		gui_params.newChild("Planning")->appendOption("hidden");
		param_modified
			<< gui_params.addBool("planning active", &string_params.do_planning)
			->describe("Perform path planning and merging on strokes");
		param_modified
			<< gui_params.addBool("Paul's version", &string_params.planning.paul);
		param_modified
			<< gui_params.addFloat("Paul length weight", &string_params.planning.k_paul_length, 0, 1);
		param_modified << gui_params.addFloat("k horizontal", &string_params.planning.k_horizontal, 0.0, 1)
			->describe("Left-right stroke weight");
		param_modified << gui_params.addFloat("k vertical", &string_params.planning.k_vertical, 0.0, 1)
			->describe("Down-stroke weight");
		//param_modified << gui_params.addFloat("k_vertical_abs", &string_params.planning.k_vertical_abs, 0.0, 1)
		//	->describe("Vertical spread (higher->less important)");
		param_modified << gui_params.addFloat("k left", &string_params.planning.k_left, 0.0, 1)
			->describe("Left-to-right weight");
		param_modified << gui_params.addFloat("k top", &string_params.planning.k_top, 0.0, 1)
			->describe("Top-down weight");
		param_modified << gui_params.addFloat("k length", &string_params.planning.k_length, 0.0, 1)
			->describe("Weights stroke length");
		param_modified << gui_params.addFloat("k distance", &string_params.planning.k_distance, 0.0, 1)
			->describe("Weights (shorter) distance between consecutive stroke end-points");
		//param_modified << gui_params.addFloat("left_weight", &string_params.planning.left_weight, 0.0, 1.)
		//	->describe("Left to right rate");
	}
	
    //child = gui_params.newChild("Stroke");

    child = gui_params.newChild("Background");
    gui_params.addColor("background color", &background_params.color);
    // clang-format on
    gui_params.newChild(name);

    if (file_exists("data/glyph_data/jap_fonts.json")) {
      json                     js       = json_from_file("data/glyph_data/jap_fonts.json");
      std::vector<std::string> fontlist = js["fonts"];
      for (auto name : fontlist)
        jap_fonts.insert(name);
    }
  }

  void load_data() {
    // can access it with params.getFloat("foo")
    load_parameters();
    load_simple_ui();

    // db.load(glyph_dir);
    // presets
    load_presets();

    // sanity checks
    string_params.simplify_eps = std::max(string_params.simplify_eps, 0.01f);
  }

  void load_presets() {
    presets.files   = cm::getFilesInFolder(preset_dir, ".xml");
    presets.current = -1;
    presets.names.clear();
    for (auto path : presets.files) {
      presets.names.push_back(getPathWithoutExt(getFilenameFromPath(path)));
    }
  }

  void menu() {
    bool must_load = false;
    bool must_save = false;
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save Parameters...", "Ctrl+S")) {
        must_save = true;
      }

      if (ImGui::MenuItem("Load Parameters..", "Ctrl+O")) {
        must_load = true;
      }

      ImGui::EndMenu();
    }

    if (must_save || shortcut(GLFW_KEY_S)) {
      std::string path;
      if (cm::saveFileDialog(path, "xml")) {
        gui_params.saveXml(path);
        load_presets();
      }
    }

    if (must_load || shortcut(GLFW_KEY_O)) {
      std::string path;
      if (cm::openFileDialog(path, "xml")) {
        gui_params.loadXml(relative_path(path));
      }
    }
  }

  bool init() {
    OPENBLAS_THREAD_SET
    mode_init();
    return true;
  }

  void exit() {
    mode_exit();
    save_parameters();
  }

  void load_paramters_keep_state(const std::string& path) {
    std::string txt = params.text;  // gui_params.addString("text",
                                    // &params.text)
    std::string cur_glyph_dir = glyph_dir;
    // int         font_index    = params.font_index;
    std::string font_name = params.font_name;
    presets.current =
        std::min(std::max(presets.current, 0), (int)presets.files.size());

    gui_params.loadXml(path);

    param_modified.force_modified = true;
    gui_params.setString("text", txt);
    // gui_params.setInt("font index", font_index);
    gui_params.setString("font name", font_name);
    // params.font_index = font_index;
    params.font_name = font_name;
    params.text      = txt;
    // glyph_dir         = cur_glyph_dir;
    // db.load(glyph_dir);  // lod_directories[current_lod]);
  }

  void load_defaults()  // overrides Demo
  {
    load_paramters_keep_state(default_configuration_path(this, "_default.xml"));
  }

  virtual void load_preset(const std::string& path) {
    load_paramters_keep_state(path);
  }

  virtual void save_preset(const std::string& path) {
    gui_params.saveXml(path);
  }

  bool gui() {
    font_modified = false;
    // Before running UI, sync C to preset if possible
    // This is for backwards compatibility with setting files that store
    // only C if (params.C_to_preset.find(string_params.C) !=
    // params.C_to_preset.end())
    // {
    //     params.schematization_preset =
    //     params.C_to_preset[string_params.C];
    // }else
    // {
    //     params.schematization_preset = -1;
    // }

    // ImGui::Text(
    //    "Parameters are still unfriendly, so demo presets are your friend");
    // ImGui::Text("Note, combo presets don't load text");
    if (ImGui::StringCombo("Saved settings", &presets.current, presets.names)) {
      load_preset(presets.files[presets.current]);
    }
    if (ImGui::IsItemHovered()) {
      std::string ttp = "Saved settings, does not load text or font\nsave to:";
      ttp += relative_path(preset_dir);
      ImGui::SetTooltip(ttp.c_str());
    }
    ImGui::SameLine();
    ImGui::PushID("Preset");
    if (presets.current > -1) {
      if (ImGui::Button("Update")) {
        save_preset(presets.files[presets.current]);
      }
      ImGui::SameLine();
    }
    if (ImGui::Button("Reload")) load_presets();
    ImGui::PopID();

    // ImGui::Text("(or press ENTER to render)");
    // ImGui::Text("up/down to switch palette");
    // ImGui::Text("shift + left/right to switch font");
    std::stringstream ss;
    ss << "Palette: " << palette.selected << " "
       << palette.names[palette.selected];
    ImGui::Text(ss.str().c_str());
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("up/down to switch palette");

    if (Keyboard::pressed(ImGuiKey_UpArrow))
      palette.selected = mod(palette.selected - 1, palette.size());
    if (Keyboard::pressed(ImGuiKey_DownArrow))
      palette.selected = mod(palette.selected + 1, palette.size());

    // if (ImGui::Button("Save png...")) trig_save_png.trigger();

    std::vector<std::string> font_names;
    for (int i = 0; i < db.font_names.size(); i++) {
      std::stringstream ss;
      std::string       fname = db.font_names[i];
      // This won't really work since we segmented "dummy" unicode for most fonts..
      //if (db.is_glyph_loadable(fname, utf8_string(std::string("\u30b9"))[0]))  //    utf8_string("\u30b9")[0]))
      //  fname = "Jap - " + fname;
      if (jap_fonts.find(fname) != jap_fonts.end())
        fname = "Jap - " + fname;
      ss << i << ": " << fname;  //db.font_names[i];
      font_names.push_back(ss.str());
      if (db.font_names[i] == params.font_name) params.font_index = i;
    }

    if (ImGui::StringCombo("Font", &params.font_index, font_names)) {
      param_modified.force_modified = true;
      font_modified                 = true;
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("shift + left/right to switch font");
    if (Keyboard::pressed(ImGuiKey_LeftArrow) && ImGui::GetIO().KeyShift) {
      params.font_index             = mod(params.font_index - 1, (int)db.font_names.size());
      param_modified.force_modified = true;
      font_modified                 = true;
    }
    if (Keyboard::pressed(ImGuiKey_RightArrow) && ImGui::GetIO().KeyShift) {
      params.font_index             = mod(params.font_index + 1, (int)db.font_names.size());
      param_modified.force_modified = true;
      font_modified                 = true;
    }

    // Sync font name
    params.font_index =
        std::min(params.font_index, (int)db.font_names.size() - 1);
    params.font_name = db.font_names[params.font_index];

    //        std::vector<std::string> lods;
    //        for (int i = 0; i < lod_directories.size(); i++)
    //            lods.push_back(format_string("%d",i));
    //        if (ImGui::StringCombo("LOD", &current_lod, lods))
    //        {
    //            param_modified.force_modified = true;
    //            db.load(lod_directories[current_lod]);
    //        }

    //        ImGui::Checkbox("Easy UI", &params.easy_ui);
    //        ImGui::SameLine();
    //        if (ImGui::Button("Reload UI"))
    //            reload_ui_json();
    //
    //    if (params.easy_ui)
    //    {
    //        simple_ui.ui(ui_json);
    //    }
    //    else
    //    {
    //        imgui(gui_params); // Creates a UI for the parameters
    //    }

    parameter_settings_gui();

    if (ImGui::Button("+ Mode options menu..."))
      ImGui::OpenPopup("param_options_popup");
    if (ImGui::BeginPopup("param_options_popup")) {
      if (ImGui::BeginMenu("Database path:")) {
        ImGui::MenuItem(glyph_dir.c_str());
        ImGui::EndMenu();
      }

      // if (ImGui::IsItemHovered()) {
      //   ImGui::SetTooltip("%s", glyph_dir.c_str());
      // }
      ImGui::Separator();
      if (ImGui::MenuItem("Save png...")) {
        trig_save_png.trigger();
      }
      ImGui::MenuItem("Transparent save", "", &params.transparent_save);
      mode_menu();
      ImGui::EndMenu();
    }

    // if (ImGui::BeginMenu("Options menu")) {
    //   // if (ImGui::MenuItem("Set glyph database...")) {  // ImGui::Button("Set
    //   //                                                  // glyph database")) {
    //   //   std::string path;
    //   //   if (openFolderDialog(path, "Select glyph database folder")) {
    //   //     glyph_dir = path;
    //   //     gui_params.setString("glyph dir", glyph_dir);
    //   //     db.load(glyph_dir);
    //   //   }
    //   // }

    //   if (ImGui::BeginMenu("Database path:")) {
    //     ImGui::MenuItem(glyph_dir.c_str());
    //     ImGui::EndMenu();
    //   }

    //   // if (ImGui::IsItemHovered()) {
    //   //   ImGui::SetTooltip("%s", glyph_dir.c_str());
    //   // }
    //   ImGui::Separator();
    //   if (ImGui::MenuItem("Save png...")) {
    //     trig_save_png.trigger();
    //   }
    //   ImGui::MenuItem("Transparent save", "", &params.transparent_save);
    //   mode_menu();
    //   ImGui::EndMenu();
    // }
    mode_gui();

    simple_or_normal_gui();
    // parameter_gui();

    // make sure schematization settings are consistent
    // int C = params.schematization_C[params.schematization_preiset];
    // if (C != -1) string_params.C = C;

    if (params.gui_active) editor.gui(true);

    return false;
  }

  void update() {
    if (simplify_modified.modified()) {
      simplify_modified.force_modified = true;
      param_modified.force_modified    = true;
      str.clear();
    }

    // Ugly sync font_index
    for (int i = 0; i < db.font_names.size(); i++) {
      if (db.font_names[i] == params.font_name) params.font_index = i;
    }
  }

  void save_state(const std::string& path) {}

  void load_state(const std::string& path) {}

  void* getData() { return &gui_params; }

  void set_line_width(double w) {
    if (view_transform)
      gfx::lineWidth(w * view_scale);
    else
      gfx::lineWidth(w);
  }

  void render() {
    glEnable(GL_LINE_SMOOTH);
    bool        save_png = false;
    std::string png_path = "";
    if (trig_save_png.isTriggered()) {
      if (saveFileDialog(png_path, "png")) save_png = true;
    }

    if (params.frame_seed)
      random::seed(params.seed);

    float w = appWidth();
    float h = appHeight();

    if (save_png && params.transparent_save) {
      gfx::setBlendMode(gfx::BLENDMODE_ALPHA);
      gfx::clear(0, 0, 0, 0);
    } else {
      gfx::setBlendMode(gfx::BLENDMODE_ALPHA);
      gfx::clear(background_params.color);
    }

    if (params.font_index >= db.font_names.size()) {
      std::cout << "Font index out of bounds\n";
      return;
    }

    if (params.animate_start_angle) {
      string_params.start_ang += params.angle_speed;
      while (string_params.start_ang > 360) string_params.start_ang -= 360;
    }

    std::string name = db.font_names[params.font_index];

    if (this->is_stroke_based) { 
      //
      
      if (editor.strokes.size())
        editor.generate_chunks(skel_params, render_data);
      editor.update_widths();
      string_params.endstrokes = editor.strokes;
      if (!save_png && params.gui_active) editor.draw_chunks();
    }
    // Estimate box and
    // bool simplify_param_modified = simplify_modified.modified();
    bool simplify_param_modified = simplify_modified.modified();
    bool parameters_changed      = simplify_modified.modified() ||
                              param_modified.modified() || !str.size() ||
                              params.rt_update || editor.isModified();
    if (parameters_changed || !str.size()) {
      string_bbox = glyph_instance_string_box(&db,
                                              name,
                                              params.text,
                                              string_params,
                                              simplify_param_modified);
      
    }

    
    // Generate string and fit to window
    double ah = appHeight();
    if (params.draw_original && shift_original) ah = ah * 0.5;
    aa_box screen_box = make_rect(0, 0, appWidth(), ah);  // appHeight());
    centerm           = eye(3, 3);
    if (str.size()) {
      centerm = rect_in_rect_transform(string_bbox, screen_box, 50);
      // aa_box rr = rect_in_rect(bbox, screen_box, 70);
      // gfx::color(1,0.5,0.);
      // gfx::draw_rect(rr);
    }

    static float rot  = 0.;
    static float zoom = 1.;
    rot += (camera.rotation - rot) * 1;  // 0.5;
    zoom += (camera.zoom - zoom) * 1;    // 0.5;
    view = eye(3, 3);
    view *= trans_2d(appCenter());
    view *= rot_2d(ag::radians(rot));  // camera.rotation));
    view *= trans_2d(camera.x, camera.y);
    view *= scaling_2d(zoom);  // camera.zoom);
    view *= trans_2d(-appCenter());

    mat viewm  = view * centerm;
    view_scale = sqrt(fabs(det(viewm(span(0, 1), span(0, 1)))));

    // view transform
    if (view_transform) gfx::pushMatrix(view * centerm);

    // Generate string
    string_params.x = 0;  // params.tx;
    string_params.y = 0;  // appHeight() / 2 + 50; //+ params.ty;
    if (parameters_changed) {
      str = generate_glyph_instance_string(&db,
                                           name,
                                           params.text,
                                           string_params,
                                           &str,
                                           simplify_param_modified);

      serifs.clear();
      if (string_params.serif_thresh > 0) {
        for (int i = 0; i < str.size(); i++) {
          serifs.push_back(glyph_serifs(str[i].info, string_params));
        }
      }
    }

    // Debug links
    // double av_w = 0.;
    // for (int i = 0; i < str.size(); i++) {
    //   double w = 0.;
    //   for (int j = 0; j < str[i].info.strokes.size(); j++)
    //     w += arma::mean(str[i].info.strokes[j].W);
    //   w /= str[i].info.strokes.size();
    //   av_w += w;

    //   gfx::pushMatrix(str[i].tm);
    //   for (int j = 0; j < str[i].info.debug_link_positions.size(); j++) {
    //     gfx::color(1, 0, 1);
    //     gfx::fillCircle(str[i].info.debug_link_positions[j], 10);
    //   }
    //   gfx::popMatrix();
    // }
    // av_w /= str.size();

    StrokeRenderData render_data_for_mode = render_data;

    bool needs_update =
        params.rt_update ||
        parameters_changed;  // Mouse::down(0) ||
                             // ImGui::GetIO().MouseDown[0];//
                             // ui::hasFocus();//params.rt_update ||
                             // editor.stroke_modified;// || editor.dirty;

    mode_render(needs_update);

    if (view_transform) gfx::popMatrix();

    // Draw original
    gfx::pushMatrix(view * centerm);

    mat om = inv(centerm) * trans_2d(0, ah * params.original_offset) * centerm;
    if (!shift_original) om = eye(3, 3);

    gfx::pushMatrix(om);

    original_shape     = instance_string_shape(str);
    //PolylineList skeletons = instance_string_skeletons(str);

    if (params.draw_original) {
      vec clr = ag::Color::complementary(background_params.color, true);
      gfx::color(clr);
      //set_line_width(params.chunk_line_width);
      gfx::lineWidth(params.chunk_line_width);
      gfx::color(0, 1.);
      gfx::draw(original_shape);
      // gfx::color(1, 0, 1, 0.5);
      // gfx::draw(skeletons);
      // gfx::lineWidth(1.);
    }
    gfx::popMatrix();
    gfx::popMatrix();
    // End draw original

    if (save_png) {
      Image img(appWidth(), appHeight(), Image::BGRA);
      gfx::color(1);
      glFlush();
      img.grabFrameBuffer();
      img.save(png_path);
    }
    // fibt
  }
};
