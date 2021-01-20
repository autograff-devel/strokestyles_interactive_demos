#pragma once
#include <boost/filesystem.hpp>

#include "GLFW/glfw3.h"
#include "ag_alpha_shape.h"
#include "ag_color.h"
#include "ag_graff_font.h"
#include "ag_imaging.h"
#include "ag_layering.h"
#include "ag_schematize.h"
#include "ag_skeletal_strokes.h"
#include "arma_ext.hpp"
#include "autograff.h"
#include "colormotor.h"
#include "common.h"
#include "deps/json.hpp"
#include "deps/tinyutf8.h"
#include "deps/utf8.h"

using json = nlohmann::json;

using namespace cm;
using namespace ag;
using namespace arma;
using namespace arma_ext;

struct StrokeReplacement {
  StrokeReplacement() {}
  StrokeReplacement(int c, int stroke_ind) {
    std::string str = utf8_string(1, (utf8_string::value_type)c).c_str();
    str += int_to_string(stroke_ind);
    name = str;
    id   = {c, stroke_ind};
  }

  json to_json() const {
    json entry;
    entry["name"]        = name;
    entry["image_id"]    = image_id;
    entry["replace"]     = json(replace);
    entry["char_ind"]    = json(id.first);
    entry["stroke_ind"]  = json(id.second);
    entry["scale_x"]     = json(params.scale_x);
    entry["scale_y"]     = json(params.scale_y);
    entry["offset_x"]    = json(params.offset_x);
    entry["offset_y"]    = json(params.offset_y);
    entry["resize_mode"] = json(params.resize_mode);
    entry["rotation"]    = json(params.rotation);
    entry["depth"]       = json(params.depth);
    entry["anim_speed"]  = json(params.anim_speed);
    entry["flip_link"]   = json(params.flip_link);

    return entry;
  }

  ipair from_json(json entry) {
    name               = entry["name"];
    image_id           = entry["image_id"];
    replace            = entry["replace"];
    id                 = {(int)entry["char_ind"], (int)entry["stroke_ind"]};
    params.scale_x     = entry["scale_x"];
    params.scale_y     = entry["scale_y"];
    params.offset_x    = entry["offset_x"];
    params.offset_y    = entry["offset_y"];
    params.resize_mode = entry["resize_mode"];
    params.rotation    = entry.value("rotation", 0.0f);
    params.depth       = entry.value("depth", 0.0f);
    params.anim_speed  = entry.value("anim_speed", 1.0f);
    params.flip_link   = entry.value("flip_link", false);
    return id;
  }

  std::string name;
  std::string image_id = "";
  bool        replace  = true;
  ipair       id;

  struct ReplacementParams {
    float scale_x     = 1.;
    float scale_y     = 1.;
    float offset_x    = 0.;
    float offset_y    = 0.;
    float rotation    = 0;
    int   resize_mode = 2;
    float depth       = 0;
    float anim_speed  = 1.;
    bool  flip_link   = false;
  };

  ReplacementParams params;

  double t = 0;
};

struct StrokeImage {
  StrokeImage(const std::string& path) {
    frames = load_gif_frames(path);
    for (auto im : frames) im->updateTexture();
    // update texture?
    // image = Image(path, Image::BGRA);
    // image.updateTexture();
    Image* image = frames[0];
    contours     = find_contours(channels(image->mat).back());
    if (!contours.size())
      box = make_rect(0, 0, image->width(), image->height());
    else
      box = bounding_rect(contours);

    name = filename_from_path(path_without_ext(path));
    // resize_mode = 2;
    //        if (rect_width(box) > rect_height(box))
    //            resize_mode = 0;
    //        else
    //            resize_mode = 1;
  }

  ~StrokeImage() {
    for (auto im : frames) delete im;
    frames.clear();
  }

  cm::Image* frame(int i) { return frames[mod(i, frames.size())]; }

  std::string             name;
  aa_box                  box;
  std::vector<cm::Image*> frames;
  // cm::Image    image;
  PolylineList contours;
};

class ImageSelectorHelper {
 public:
  std::map<std::string, std::string>  paths;
  std::map<std::string, StrokeImage*> images;
  std::string                         path;

  ImageSelectorHelper() : path("./data/images/stroke_replacements_daichi") {}

  ~ImageSelectorHelper() {
    clear_images();
    clear_replacements();
  }

  void clear_replacements() {
    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
      delete it->second;
    }

    replacements.clear();
    deselect();
  }

  void clear_images() {
    for (auto it = images.begin(); it != images.end(); ++it)
      if (it->second) delete it->second;
    images.clear();
    paths.clear();
  }

  void reload() {
    this->path = relative_path(this->path);
    clear_images();
    std::vector<std::string> full_paths =
        files_in_directory(this->path, {"png", "gif"});  //, "png");

    for (const std::string& path : full_paths) {
      std::string key   = filename_from_path(path_without_ext(path));
      this->paths[key]  = path;
      this->images[key] = 0;
    }
  }

  StrokeImage* image(const std::string& key) {
    if (images.find(key) == images.end()) {
      std::cout << "No image with key " << key << std::endl;
      return 0;
    }

    StrokeImage* im = images[key];
    if (im == 0) {
      im          = new StrokeImage(this->paths[key]);
      images[key] = im;
    }

    return im;
  }

  void init() { reload(); }

  bool button(StrokeImage* im, bool selected) {
    ImVec2 size(50, 50);
    ImVec4 bg_color = ImVec4(0.5, 0.5, 0.5, 1.);
    if (selected) bg_color = ImVec4(1., 1., 1., 1.);
    void* id;
    if (im) {
      id = (void*)im->frames[0]->tex.getId();
    } else {
      id = (void*)-1;
    }

    if (!im) return ImGui::Button("No Image", size);

    return ImGui::ImageButton(id,
                              size,
                              ImVec2(0, 0),
                              ImVec2(1, 1),
                              -1,
                              bg_color);
  }

  std::string image_gui(std::string selected) {
    ImGui::Text((std::string("Image dir: ") + path).c_str());
#define GRIDLINE \
  if (((k++) % 4) < 3) ImGui::SameLine();
    if (ImGui::Button("Reload images")) {
      reload();
    }

    int k = 0;
    if (button(0, selected == "")) selected = "";
    GRIDLINE
    for (auto it = images.begin(); it != images.end(); ++it) {
      StrokeImage* im = image(it->first);
      if (button(im, it->first == selected)) selected = it->first;
      if (ImGui::IsItemHovered()) ImGui::SetTooltip(im->name.c_str());

      GRIDLINE
    }

    return selected;
  }

  static bool item_getter(void* data, int idx, const char** out_text) {
    if (idx < 0) return false;
    ImageSelectorHelper* inst = (ImageSelectorHelper*)data;
    *out_text                 = inst->flat_replacements[idx]->name.c_str();
    return true;
  }

  void select(int c, int stroke_ind) { selected = {c, stroke_ind}; }

  void select(ipair id) { selected = id; }

  bool none_selected() const { return selected.first == -1; }
  void deselect() {
    selected             = {-1, -1};
    selected_replacement = 0;
  }
  bool is_selected(int c, int stroke_ind) const {
    return selected.first == c && selected.second == stroke_ind;
  }

  void gui() {
    save_selection = false;

    ImGui::Begin("Replacement UI");
    bool reset_image_dir = false;
    if (ImGui::Button("Set Image Directory...")) {
      if (openFolderDialog(path, "Select Directory")) {
        path            = relative_path(path);
        reset_image_dir = true;
        reload();
      }
    }
    std::vector<ipair> keys;
    flat_replacements.clear();
    int current          = -1;
    selected_replacement = 0;
    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
      if (it->first == selected) current = flat_replacements.size();
      flat_replacements.push_back(it->second);
      keys.push_back(it->first);
    }

    if (flat_replacements.size()) {
      ImGui::Text("Replaced strokes: select here or by clicking on strokes");
      if (ImGui::ListBox("Replacements",
                         &current,
                         item_getter,
                         this,
                         flat_replacements.size(),
                         5)) {
        selected = keys[current];
      }
    }

    std::string image_id = "";

    if (none_selected()) {
      ImGui::Text("To replace a stroke with an image:");
      ImGui::Text("click on a stroke of the font, and then select an image");
      ImGui::End();
      return;
    }

    if (ImGui::Button("Deselect")) deselect();

    ImGui::Checkbox("Automatically load/save defaults", &load_default_replacements);
    ImGui::SameLine();
    if (ImGui::Button("<- Clear"))
      default_replacement_params.clear();

    if (current > -1) {
      image_id = flat_replacements[current]->image_id;

      if (ImGui::Button("Delete Selected")) {
        delete replacements[keys[current]];
        replacements.erase(keys[current]);
      }

      ImGui::SameLine();
      if (ImGui::Button("Delete All")) {
        clear_replacements();
      }

      StrokeReplacement* r =
          get_replacement(keys[current].first, keys[current].second);
      selected_replacement = r;
      if (r) {
        bool viz = ImGui::CollapsingHeader("Transform",
                                           ImGuiTreeNodeFlags_AllowItemOverlap |
                                               ImGuiTreeNodeFlags_DefaultOpen);
        if (viz) {
          std::vector<std::string> align = {"horizontal", "vertical", "both"};
          ImGui::StringCombo("Scaling", &r->params.resize_mode, align);
          if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Alignment of input image to stroke, default is 'both', but "
                "usually 'horizontal' or 'vertical' works better");

          ImGui::SliderFloat("scale x", &r->params.scale_x, 0.1, 2.);
          ImGui::SliderFloat("scale y", &r->params.scale_y, 0.1, 2.);
          ImGui::SliderFloat("offset x", &r->params.offset_x, -100, 100);
          ImGui::SliderFloat("offset y", &r->params.offset_y, -100, 100);
          ImGui::SliderFloat("rotation", &r->params.rotation, 0, 360);
          ImGui::SliderFloat("depth", &r->params.depth, 0, 1000);
          ImGui::SliderFloat("anim speed", &r->params.anim_speed, -10, 10);
          ImGui::Checkbox("flip when T junction", &r->params.flip_link);
          if (ImGui::Button("Stop anim")) r->params.anim_speed = 0;
        }

        default_replacement_params[r->image_id] = r->params;
      }
    }

    image_id = image_gui(image_id);

    if (current > -1) {
      flat_replacements[current]->image_id = image_id;

    } else {
      if (image_id != "") {
        assert(replacements.find(selected) == replacements.end());
        replacements[selected] =
            new StrokeReplacement(selected.first, selected.second);

        replacements[selected]->image_id = image_id;
        StrokeImage* im                  = image(image_id);
        if (rect_width(im->box) > rect_height(im->box))
          replacements[selected]->params.resize_mode = 0;
        else
          replacements[selected]->params.resize_mode = 1;

        if (load_default_replacements)
          if (default_replacement_params.find(image_id) != default_replacement_params.end())
            replacements[selected]->params = default_replacement_params[image_id];
      }
    }

    ImGui::Text(
        "Use this to export stroke image and then edit in photoshop. Reload "
        "Images will realod it here");
    if (ImGui::Button("Export stroke image")) {
      if (saveFileDialog(save_path, "png")) {
        save_selection = true;
        reload();
      }
    }

    ImGui::End();
  }

  StrokeReplacement* get_replacement(int c, int stroke_ind) {
    ipair key = {c, stroke_ind};
    auto  it  = replacements.find(key);
    if (it != replacements.end()) return it->second;
    return 0;
  }

  StrokeImage* get_stroke_image(int c, int stroke_ind) {
    StrokeReplacement* r = get_replacement(c, stroke_ind);
    if (!r) return 0;
    return image(r->image_id);
  }

  json to_json() const {
    json res;
    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
      res.push_back(it->second->to_json());
    }
    return res;
  }

  void update_default_params(StrokeReplacement* r) {
    if (!load_default_replacements)
      return;
    default_replacement_params[r->image_id] = r->params;
  }

  void from_json(json entry) {
    clear_replacements();
    for (auto rentry : entry) {
      StrokeReplacement* r  = new StrokeReplacement();
      ipair              id = r->from_json(rentry);
      replacements[id]      = r;
      update_default_params(r);
    }
  }

  void save_json(const std::string& path) {
    json        js      = to_json();
    std::string jsonstr = js.dump();
    ag::string_to_file(jsonstr, path);
  }

  void load_json(const std::string& path) {
    std::string jsonstr = string_from_file(path);
    if (jsonstr.length()) {
      json parsed = json::parse(jsonstr);
      from_json(parsed);
    }
  }

  void animate(double dt) {
    for (auto r : flat_replacements) r->t += r->params.anim_speed * dt;
  }

  void reset_anim() {
    for (auto r : flat_replacements) r->t = 0;
  }

  std::map<std::string, StrokeReplacement::ReplacementParams> default_replacement_params;
  bool                                                        load_default_replacements = true;
  std::map<ipair, StrokeReplacement*>                         replacements;
  std::vector<StrokeReplacement*>                             flat_replacements;
  ipair                                                       selected             = {-1, -1};
  StrokeReplacement*                                          selected_replacement = 0;
  bool                                                        save_selection       = false;
  std::string                                                 save_path;
};

class StrokeSimilarity {
 public:
  StrokeSimilarity() { distances = zeros(0, 0); }

  bool load(const std::string& path) {
    index_map.clear();
    groups.clear();

    valid               = false;
    std::string jsonstr = string_from_file(path);
    if (jsonstr == "") return false;

    json parsed = json::parse(jsonstr);
    distances   = json_to_mat(parsed["distances"]);

    index_map.clear();
    json char_indices = parsed["char_indices"];
    for (json::iterator it = char_indices.begin(); it != char_indices.end();
         ++it) {
      int  key       = utf8_string(std::string(it.key()))[0];
      ivec inds      = json_to_ivec(it.value());
      index_map[key] = inds;
    }

    valid = true;
    return true;
  }

  bool is_valid() const { return distances.n_rows != 0; }

  void cluster(double thresh) {
    if (!valid) return;

    // pairwise distance vector
    int   m = distances.n_rows;
    vec   Y(m * (m - 1) / 2);
    uword k = 0;
    for (int i = 0; i < m; i++)
      for (int j = i + 1; j < m; j++) Y(k++) = distances(i, j);

    std::cout << "Computing clustering for thresh " << thresh << std::endl;
    mat  Z        = linkage(Y);
    uvec clusters = arma_ext::cluster(Z, thresh);
    std::cout << "Done" << std::endl;
    this->groups.clear();
    for (int i = 0; i < clusters.n_elem; i++) {
      groups.push_back(clusters(i));
    }
  }

  int get_id(int c, int stroke_ind) {
    if (index_map.find(c) == index_map.end()) return -1;
    ivec inds = index_map[c];
    if (stroke_ind >= inds.n_elem) return -1;
    return groups[inds[stroke_ind]];
  }

  std::map<int, ivec> index_map;
  std::vector<int>    groups;
  arma::mat           distances;
  bool                valid = false;
};

class FontStylizationSimilarity : public FontStylizationBase {
 public:
  int tool = 0;

  // cm::ParamModifiedTracker param_modified;
  cm::ParamModifiedTracker cluster_param_modified;

  // std::string data_dir =
  //     "/Users/colormotor/data/graffitype/glyph_models_new/"
  //     "params_T_0.2_lambda_0.5_beta_0.5_CM_0.2_nested_0/data/";
  struct {
    bool  show_similarities = true;
    float cluster_thresh    = 0.5;
    float dt                = 0.01;
    float id_offset         = 0;
  } mode_params;

  struct {
    Image test_img;
  } data;

  ImageSelectorHelper image_selector;

  std::string selected_image = "";

  StrokeSimilarity similarity;
  int              current_lod = 0;

  enum {
    MODE_SELECT = 0,
  };

  FontStylizationSimilarity()
      : FontStylizationBase("Similarity Font Stylization",
                            "./data/presets/stylization_similarity",
                            false /*is_stroke_based*/) {
    ParamList* child;
    // gui_params.addBool("debug draw", &params.debug_draw)->appendOption("n");
    gui_params.addBool("show similarities (disable for rendering)",
                       &mode_params.show_similarities);
    gui_params.addString("image path", &image_selector.path)->noGui();
    gui_params.addFloat("dt", &mode_params.dt, -1, 1);
    gui_params.addFloat("id offset", &mode_params.id_offset, 0, 100);
    gui_params.newChild("Clustering");
    cluster_param_modified << gui_params.addFloat("thresh",
                                                  &mode_params.cluster_thresh,
                                                  0.01,
                                                  1.);

    data.test_img = Image("./data/images/tests/test_trasp.png", Image::BGRA);
    load_data();  // <- load_parameters() : load_params_
    load_distances();
    params.gui_active = false;  // Editor
                                //
    image_selector.reload();
  }

  void load_distances() {
    // if (params.font_index >= db.font_names.size()) return;
    std::string name = params.font_name;  // db.font_names[params.font_index];
    similarity.load(
        join_path(get_data_dir(), "stroke_distances/" + name + ".json"));
    if (similarity.is_valid()) similarity.cluster(mode_params.cluster_thresh);
  }

  std::string get_data_dir() const { return parent_directory(glyph_dir); }

  std::string get_json_preset_path(const std::string& xml_path) {
    return path_without_ext(xml_path) + ".json";
  }

  virtual bool save_params_request(const std::string& path) {
    gui_params.saveXml(path);
    image_selector.save_json(get_json_preset_path(path));
    return true;
  }

  virtual void load_params_request(const std::string& path) {
    gui_params.loadXml(path);
    param_modified.force_modified = true;
    image_selector.clear_replacements();
    image_selector.reload();
    image_selector.load_json(get_json_preset_path(path));
    load_distances();
  }

  void load_preset(const std::string& path)  //
  {
    load_params_request(path);
  }

  void save_preset(const std::string& path)  //
  {
    save_params_request(path);
  }

  void mode_gui() {
    params.gui_active = false;  // Editor
    if (font_modified) {
      load_distances();
      image_selector.clear_replacements();
      param_modified.force_modified = true;
    }
    Param* pp = gui_params.find("draw original");
    if (pp->isDirty()) {
      if (!pp->getBool())
        image_selector.deselect();
    }
    if (ImGui::Button("Reset anim")) image_selector.reset_anim();

    image_selector.gui();  // selected_image);
                           //
  }

  void draw_replacement(StrokeReplacement* r, const PolylineList& shape, int flipflag) {
    if (!r) return;
    StrokeImage* im = image_selector.image(r->image_id);
    if (!im) return;

    mat flipmat = eye(3, 3);
    if (flipflag == 2)
      flipmat = ag::scaling_2d(-1, 1, true);
    else if (flipflag == 1)
      flipmat = ag::scaling_2d(1, -1, true);

    aa_box dst  = bounding_rect(shape);
    aa_box src  = im->box;
    mat    m    = rect_in_rect_side_transform(src, dst, 0, r->params.resize_mode);
    vec    cenp = rect_center(src);
    m           = m * trans_2d(cenp) * rot_2d(ag::radians(r->params.rotation)) *
        scaling_2d(r->params.scale_x, r->params.scale_y) *
        trans_2d(r->params.offset_x, r->params.offset_y) * flipmat * trans_2d(-cenp);
    gfx::pushMatrix(m);
    gfx::color(1);
    im->frame((int)r->t)->draw(0, 0);
    gfx::popMatrix();
  }

  void mode_render(bool needs_update) {
    params.rt_update = false;

    if (cluster_param_modified.modified()) {
      similarity.cluster(mode_params.cluster_thresh);
    }

    random::seed(params.seed);

    float w = appWidth();
    float h = appHeight();

    gfx::setBlendMode(gfx::BLENDMODE_ALPHA);
    gfx::clear(background_params.color);

    PolylineList shape = instance_string_shape(str);
    if (mode_params.show_similarities) gfx::draw(shape);

    utf8_string text = params.text;

    std::map<int, StrokeReplacement*> replacement_map;
    std::vector<int>                  ids;

    std::vector<PolylineList> all_stroke_shapes;
    std::set<int>             ids_unique;

    for (int i = 0; i < str.size(); i++) {
      for (int j = 0; j < str[i].info.strokes.size(); j++) {
        int id = similarity.get_id(text[i], j);
        ids.push_back(id);
        ids_unique.insert(id);

        PolylineList stroke_shape =
            shape_offset(str[i].info.strokes[j].shape, 2);
        all_stroke_shapes.push_back(stroke_shape);

        StrokeReplacement* r = image_selector.get_replacement(text[i], j);
        if (replacement_map.find(id) == replacement_map.end() && r) {
          replacement_map[id] = r;
        }
      }
    }

    std::map<int, vec> id_to_color;
    int                c = 0;
    for (auto id : ids_unique) {
      id_to_color[id] = ag::Color::palette_categorical(c + mode_params.id_offset, 0.7);
      c++;
    }

    int                             k = 0;
    std::vector<double>             depths;
    std::vector<StrokeReplacement*> replacements;
    std::vector<int>                link_flip_flags;
    std::vector<PolylineList>       stroke_shapes;

    std::vector<arma::mat> tms;
    int                    hover_id    = -1;
    int                    selected_id = -1;

    bool clicked = Mouse::clicked(0);

    for (int i = 0; i < str.size(); i++) {
      gfx::pushMatrix(str[i].tm);
      std::map<int, std::vector<Link>> stroke_links;  // links ending in a given stroke
      for (auto link : str[i].info.connectivity.links) {
        stroke_links[link.linked_stroke].push_back(link);
      }

      for (int j = 0; j < str[i].info.strokes.size(); j++) {
        PolylineList stroke_shape = all_stroke_shapes[k];

        vec clr = {0.8, 0.8, 0.8, 1.};
        int id =
            ids[k];  //similarity.get_id(text[i], j);  // cluster(params.cluster_thresh);
        clr = id_to_color[id];
        if (!mode_params.show_similarities) clr = {0., 0., 0., 1.};

        if (image_selector.is_selected(text[i], j)) {
          selected_id = id;
        }

        if (replacement_map.find(id) != replacement_map.end()) {
          StrokeReplacement* r = replacement_map[id];

          // if (r == image_selector.selected_replacement) {
          //   selected_id = id;  //ids[k];
          // }

          if (r) {
            auto it = stroke_links.find(j);
            if (it != stroke_links.end() && it->second.size() == 1) {
              aa_box box = bounding_rect(str[i].info.strokes[j].P);
              double w   = rect_width(box);
              double h   = rect_height(box);
              vec    d;
              int    dir = 0;
              if (w > h) {
                d   = vec({1, 0});
                dir = 1;
              } else {
                d   = vec({0, 1});
                dir = 2;
              }
              vec dlink;

              Link            l = it->second[0];  //stroke_links[j];
              const Polyline& Q = str[i].info.strokes[l.link_stroke].P;
              if (l.link_terminal == 0) {
                dlink = ag::normalize(Q[0] - Q[1]);
              } else {
                dlink = ag::normalize(Q[Q.size() - 1] - Q[Q.size() - 2]);
              }
              if (dot(d, dlink) < 0 && r->params.flip_link)
                link_flip_flags.push_back(dir);
              else
                link_flip_flags.push_back(0);
            } else {
              link_flip_flags.push_back(0);
            }
            // draw_replacement(r, stroke_shape);
            replacements.push_back(r);
            stroke_shapes.push_back(stroke_shape);
            depths.push_back(r->params.depth);
            tms.push_back(str[i].tm);
          }
        } else {
          gfx::color(clr);
          gfx::fill(stroke_shape);
        }

        // Test for mouse input
        vec mp = affine_mul(inv(view * centerm * str[i].tm), Mouse::pos());

        bool made_selection = false;
        if (point_in_shape(mp, stroke_shape)) {
          hover_id = similarity.get_id(text[i], j);
          if (clicked) {
            image_selector.select(text[i], j);
            made_selection = true;
            if (replacement_map.find(id) != replacement_map.end())
              image_selector.select(replacement_map[id]->id);
            clicked = false;
          }
        }

        //                if (!made_selection && clicked)
        //                {
        //                    image_selector.deselect();
        //                }

        if (image_selector.is_selected(text[i], j)) {
          gfx::color(0, 1, 0);
          gfx::draw(stroke_shape);

          if (image_selector.save_selection) {
            Rasterizer2D rast(512, 512);
            rast.fill_shape_fitted(stroke_shape);
            // rast.fill_shape(str[i].info.strokes[j].shape);
            rast.rasterize();
            cv::Mat im = rast.to_black_transparent_mat();
            Image   img(im);
            img.save(image_selector.save_path);
            SvgWriter svg(path_without_ext(image_selector.save_path) + ".svg");
            svg.color(0, 0, 0);
            svg.fill(stroke_shape);
            svg.save();
          }
        }
        ++k;
      }
      gfx::popMatrix();
    }

    image_selector.animate(mode_params.dt);

    gfx::setBlendMode(gfx::BLENDMODE_ALPHA_PREMULTIPLIED);
    vec  D = vec(depths);
    uvec I = sort_index(D);
    for (auto i : I) {
      gfx::pushMatrix(tms[i]);
      draw_replacement(replacements[i], stroke_shapes[i], link_flip_flags[i]);
      gfx::popMatrix();
    }
    gfx::setBlendMode(gfx::BLENDMODE_ALPHA);
    gfx::color(1);

    k = 0;
    for (int i = 0; i < str.size(); i++) {
      gfx::pushMatrix(str[i].tm);
      for (int j = 0; j < str[i].info.strokes.size(); j++) {
        const PolylineList& stroke_shape = all_stroke_shapes[k];
        int                 ns           = stroke_shape.size();
        if (ns) {
          if (hover_id == ids[k]) {
            gfx::color(1., 0., 0., 0.8);
            gfx::fill(stroke_shape);
          }
          gfx::lineWidth(params.chunk_line_width);
          if (selected_id == ids[k]) {
            gfx::color(1, 0, 0);
            gfx::draw(stroke_shape);
          }
          gfx::lineWidth(1.);
        }

        ++k;
      }
      gfx::popMatrix();
    }
    //        StrokeImage *im = image_selector.image(selected_image);
    //        if (im)
    //        {
    //            im->draw(0,0);
    //            PolylineList contours = find_contours(channels(im->mat)[3]);
    //            gfx::color(0,0.5, 1.);
    //            gfx::draw(contours);
    //        }
  }
};
