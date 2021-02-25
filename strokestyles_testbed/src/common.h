#pragma once
#include "ag_color.h"
#include "ag_colormotor.h"
#include "ag_graff_font.h"
#include "ag_graff_primitives.h"
#include "ag_json.h"
#include "autograff.h"
#include "colormotor.h"
#include "gfx_ui.h"

using namespace cm;
using namespace arma;

namespace ag {

//#define OPENBLAS_THREAD_SET setenv("OPENBLAS_NUM_THREADS", "1", true);
#define OPENBLAS_THREAD_SET

/// returns wether a keynoard shortcut in pressed
inline bool shortcut(int c) {
  ImGuiIO& io = ImGui::GetIO();
  // printf("%c pressed: %d\n", (char)c, ImGui::IsKeyPressed(c));
  return io.KeySuper && Keyboard::pressed(c, false);
}

inline std::string configuration_path(AppModule* app, const std::string& ext) {
  return "data/configurations/" + app->filename() + ext;
}

inline std::string default_configuration_path(AppModule* app, const std::string& ext) {
  return "data/default_configurations/" + app->filename() + ext;
}

#define MAX_PALETTE 1000

class PaletteManager {
 public:
  PaletteManager(const std::string& path) : _cur(-1), selected(0) {
    paths = cm::getFilesInFolder(path);
    for (auto path : paths) {
      names.push_back(getFilenameFromPath(getPathWithoutExt(path)));
    }

    vec clr        = Color::rgb_to_hcl({0., 0., 0., 1.});
    lowpass_colors = zeros(4, MAX_PALETTE);
    for (int i = 0; i < MAX_PALETTE; i++) lowpass_colors.col(i) = clr;
  }

  arma::vec random(double alpha = 1.) {
    update();
    return palette.random(alpha);
  }
  arma::vec color(int index, double alpha = 1.) {
    update();
    return palette.color(index, alpha);
  }

  arma::vec color_lowpass(int index, double alpha = 1.) {
    update();
    if (!palette.colors.size()) {
      return {1., 1., 1., alpha};
    }
    vec clr =
        Color::hcl_to_rgb(lowpass_colors.col(index % palette.colors.size()));
    clr[3] = alpha;
    return clr;
  }

  void update_lowpass(double k) {
    for (int i = 0; i < palette.colors.size(); i++) {
      vec src               = lowpass_colors.col(i);
      vec dst               = Color::rgb_to_hcl(palette.color(i));
      vec clr               = src + (dst - src) * k;
      clr[3]                = 1;
      lowpass_colors.col(i) = clr;  // Color::hcl_to_rgb(clr);
    }
  }

  void draw(const arma::vec& p, double swatch_size);

  int size() const { return names.size(); }

  std::vector<std::string> names;
  std::vector<std::string> paths;
  int                      selected;
  ASEPalette               palette;
  arma::mat                lowpass_colors;

  void update() {
    if (_cur != selected) {
      selected = selected % paths.size();
      palette  = ASEPalette(paths[selected]);
    }
    _cur = selected;
  }

 private:
  int _cur;
};
}  // namespace ag

class ContourMaker {
 public:
  ag::Polyline pts;
  int     selected = -1;

  ContourMaker() { pts = ag::Polyline(); }

  void interactDrag() {}

  void interactDraw() {
    if (Mouse::clicked(0)) {
      pts.add(Mouse::pos());
    }
  }

  void interact(int tool) {
    if (Mouse::clicked(0)) selected = -1;

    for (int i = 0; i < pts.size(); i++) {
      pts[i] = ui::dragger(i, pts[i]);
      if (ui::modified()) selected = i;
    }

    switch (tool) {
      case 0:
        // drag
        interactDrag();
        break;
      case 1:
        // add
        interactDraw();
        break;
    }
  }

  void draw(bool closed = false) { gfx::draw(pts); }

  void save(const std::string& path) 
  { 
    if (pts.size())
      ag::json_to_file(ag::polyline_to_json(pts), path);
  }

  void load(const std::string& path) 
  { 
    if (!ag::file_exists(path))
    {
      std::cout << path << " does not exist!\n";
      return;
    }
    pts = ag::json_to_polyline(ag::json_from_file(path)); 
  }

  void clear() {
    pts      = ag::Polyline();
    selected = -1;
  }
};

namespace ag {

// struct Stroke
//{
//    Polyline points;
//    arma::mat W;
//    arma::vec smooth_vals;
//    bool start_arrow;
//    bool end_arrow;
//};

struct Arrows {
  bool  a;
  bool  b;
  ipair union_a;
  ipair union_b;
};

inline void contrast_to_widths(float* w1, float* w2, float contrast) {
  *w2 = 0.5 + 0.4 * contrast;
  *w1 = 1. - *w2;
}

inline void widths_to_contrast(float* contrast, float w1, float w2) {
  *contrast = (w2 - 0.5) / 0.4;
}

std::string relative_path(const std::string& full_path);
void        create_directory(const std::string& path);

/// Basic glyph editor, allows to edit and construct a combination of stylized
/// strokes and control their layering
class GlyphEditor {
 public:
  StrokeInfo default_stroke;

  std::vector<StrokeInfo> strokes;
  StylizedGlyph           glyph;
  ArrowHeadParams         arrow_params;
  arma::ivec              z_orders;

  std::vector<int> partitions;
  // std::vector<ipair> unions;

  int selected_stroke  = 0;
  int selected_point   = -1;
  int selected_segment = -1;

  float miter_limit = 1.5;

  int  tool            = 0;
  bool dirty           = false;
  bool stroke_modified = false;
  bool modified        = false;

  float default_width = 50;

  struct {
    int   C         = 0;
    float start_ang = 0;
  } schematization_params;

  GlyphEditor() {
    default_stroke.style.null = false;
    default_stroke.style.w0   = 0.2;
    default_stroke.style.w1   = 0.8;
    default_stroke.style.w    = default_width;

    default_stroke.style.chisel_ang = 1.3;
    default_stroke.style.spine_type = StrokeStyle::SPINE_CHISEL;
    default_stroke.style.periodic   = false;
    dirty                           = false;
  }

  void deselect_all() {
    selected_stroke  = 0;
    selected_point   = -1;
    selected_segment = -1;
  }

  void deselect_local() {
    selected_segment = -1;
    selected_point   = -1;
  }

  void interact_drag() {
    //        if (selected_point > -1)
    //        {
    //            vec mp = Mouse::pos();
    //            GraffStrokeInfo& stroke = strokes[selected_stroke];
    //            stroke.smooth_vals(selected_point) =
    //            ag::clamp(ag::distance(stroke.spine_points[selected_point],
    //            Mouse::pos())/200., 0., 2.);
    //        }
  }

  void update_vertex_widths() {
    for (int i = 0; i < strokes.size(); i++) {
      const vec& W = strokes[i].W;
      if (W.n_elem) strokes[i].Wv = join_vert(W, ones(1) * W(W.n_elem - 1));
    }
  }

  void interact_draw() {
    if (Mouse::clicked(0)) {
      this->modified = true;

      if (!strokes.size()) {
        strokes.push_back(default_stroke);
        selected_stroke = 0;
      }

      StrokeInfo& stroke = strokes[selected_stroke];
      stroke.P.add(Mouse::pos());
      // (m - 1) data
      if (stroke.P.size() > 1) {
        // Widths
        if (stroke.W.n_elem) {
          stroke.W =
              join_cols(stroke.W, ones(1) * stroke.W(stroke.W.n_elem - 1));
        } else {
          stroke.W = ones(1) * 100;
        }

        // stroke.width_multipliers = ones(2, stroke.W.n_elem);

        if (partitions.size())
          partitions.push_back(partitions.back() + 1);
        else
          partitions.push_back(0);

        // Widths
        if (z_orders.n_elem) {
          z_orders = join_cols(z_orders, ones<ivec>(1) * (z_orders.max() + 1));
        } else {
          z_orders = zeros<ivec>(1);
        }
      }

      // (m) data
      if (stroke.corner_radii.n_elem) {
        stroke.corner_radii = join_cols(
            stroke.corner_radii,
            ones(1) * stroke.corner_radii(stroke.corner_radii.n_elem - 1));
        stroke.corner_flags = join_cols(stroke.corner_flags, ones<ivec>(1));
      } else {
        stroke.corner_radii = ones(1);
        stroke.corner_flags = zeros<ivec>(1);
      }
    }
  }

  void flip_up(const std::vector<int> layers, int a, int b) {
    if (layers.size() == 2) {
      std::swap(z_orders[a], z_orders[b]);
      return;
    }

    int next = layers[1];
    while (z_orders[a] < z_orders[b]) {
      std::swap(z_orders[a], z_orders[next]);
      if (layers.size() > 2) {
        std::vector<int> L(layers.begin() + 1, layers.end());

        flip_up(L, a, b);
      }
    }
  }

  // void flip_up(const std::vector<int> layers, int a, int b) {
  //   int i = 0;
  //   for (int j : layers) {
  //     if (z_orders[j] < z_orders[i]) i = j;
  //   }

  //   z_orders[i] = z_orders.max() + 1;
  //   uvec z      = sort_index(z_orders);
  //   for (int j = 0; j < z.n_elem; j++) z_orders[j] = z[j];

  //   // if (layers.size() == 2)
  //   // {
  //   //     std::swap(z_orders[a], z_orders[b]);
  //   //     return;
  //   // }

  //   // int next = layers[1];
  //   // while (z_orders[a] < z_orders[b])
  //   // {
  //   //     std::swap(z_orders[a], z_orders[next]);
  //   //     if (layers.size()>2)
  //   //     {
  //   //         std::vector<int> L(layers.begin()+1, layers.end());

  //   //         flip_up(L, a, b);
  //   //     }
  //   // }
  // }

  bool remove_unions(const std::vector<int> layers) {
    int           found = false;
    std::set<int> to_remove;
    std::set<int> L;
    for (int i = 0; i < layers.size(); i++) {
      L.insert(layers[i]);
    }

    for (int i = 0; i < glyph.unions.size(); i++) {
      ipair u = glyph.unions[i];
      if (L.find(u.first) != L.end() && L.find(u.second) != L.end())
        to_remove.insert(i);
    }

    if (!to_remove.size()) return false;

    std::vector<ipair> new_unions;
    for (int i = 0; i < glyph.unions.size(); i++) {
      if (to_remove.find(i) == to_remove.end())
        new_unions.push_back(glyph.unions[i]);
    }
    glyph.unions = new_unions;
    return true;
  }

  void add_unions(const std::vector<int> layers) {
    int m = layers.size();
    for (int i = 0; i < m; i++)
      for (int j = 0; j < m; j++) {
        if (i == j) continue;
        glyph.unions.push_back({layers[i], layers[j]});
      }
  }

  void interact_layering() {
    vec mp = Mouse::pos();

    int              k = 0;
    std::vector<int> selected_partitions;

    bool clicked = Mouse::clicked(0);
    this->dirty  = clicked;

    for (int i = 0; i < glyph.chunks.size(); i++) {
      const Chunk&                     chunk = glyph.chunks[i];
      const std::vector<PolylineList>& partition_shapes =
          chunk.layering_data.partition_shapes;

      for (int j = 0; j < partition_shapes.size(); j++) {
        if (point_in_shape(mp, partition_shapes[j])) {
          gfx::color(0, 0.5, 1, 0.4);
          gfx::fill(partition_shapes[j]);
          if (clicked) {
            selected_partitions.push_back(k);
          }
        }
        ++k;
      }
    }

    if (selected_partitions.size() < 2) return;

    this->modified = true;

    if (tool == 2) {
      //        uvec I = sort_index()
      //        for (int i = min_partition; i <= max_partition; i++ )
      //            selected_partitions.push_back(i);
      //
      std::sort(selected_partitions.begin(),
                selected_partitions.end(),
                [=](int ia, int ib) { return z_orders[ia] > z_orders[ib]; });

      // std::swap(z_orders[selected_partitions[0]],
      // z_orders[selected_partitions.back()]);
      flip_up(selected_partitions,
              selected_partitions[0],
              selected_partitions.back());

      // z_orders.t().print();
      //        std::vector layers = sorted(layers, key=lambda i: self.Zs[i])
      //            self.Zs[layers[0]], self.Zs[layers[-1]] =
      //            self.Zs[layers[-1]], self.Zs[layers[0]]
      //
    } else if (tool == 3) {
      if (!remove_unions(selected_partitions)) add_unions(selected_partitions);
    }
  }

  //    void remove_last_stroke_point()
  //    {
  //        GraffStrokeInfo& stroke = strokes[selected_stroke];
  //        if (!stroke.spine_poits.size())
  //            return;
  //
  //        stroke.spine_points.pop();
  //
  //    // (m - 1) data
  //        if (stroke.spine_points.size()>2)
  //        {
  //            stroke.W = stroke.W(0, stroke.W.n_elem-2);
  //        }
  //        else
  //        {
  //            stroke.W = zeros(0);
  //        }
  //
  //        if (partitions.size())
  //            partitions.pop();
  //
  //
  //        stroke.width_multipliers = ones(2, stroke.W.n_elem);
  //
  //        if (partitions.size())
  //            partitions.push_back(partitions.back()+1);
  //        else
  //            partitions.push_back(0);
  //
  //        // Widths
  //        if (z_orders.n_elem)
  //        {
  //            z_orders = join_cols(z_orders,
  //            ones<ivec>(1)*(z_orders.max()+1));
  //        }
  //        else
  //        {
  //            z_orders = zeros<ivec>(1);
  //        }
  //
  //    }

  void interact(int tool_ = -1) {
    if (tool_ != -1) tool = tool_;

    if (Mouse::clicked(0)) deselect_local();

    this->dirty = false;

    // discard empty strokes
    std::vector<StrokeInfo> tmp = strokes;
    strokes.clear();
    for (int i = 0; i < tmp.size(); i++) {
      if (tmp[i].P.size() || i == tmp.size() - 1) {
        strokes.push_back(tmp[i]);
      } else {
        deselect_all();
        assert(false);  // <- need to implement clearing of a stroke
      }
    }

    //        if (Keyboard::pressed(32))
    //        {
    //            std::cout << "asdasd";
    //        }
    //
    //        if (Keyboard::pressed(ImGuiKey_Tab))
    //        {
    //            std::cout << "asddd asd";
    //        }
    //
    //
    if (Keyboard::pressed(32) && strokes.size() &&
        strokes.back().P.size() > 1) {
      strokes.push_back(default_stroke);
      selected_stroke = strokes.size() - 1;
    }

    //        if (Keyboard::pressed(ImGuiKey_Backspace) && strokes.size() &&
    //        selected_stroke >= 0 && selected_stroke < strokes.size()
    //        {
    //            strokes.push_back(default_stroke);
    //            selected_stroke = strokes.size()-1;
    //        }

    // edit spine points
    int  k        = 0;
    bool modified = false;
    for (int i = 0; i < strokes.size(); i++) {
      for (int j = 0; j < strokes[i].P.size(); j++) {
        strokes[i].P[j] = ui::dragger(k++, strokes[i].P[j]);

        if (ui::modified()) {
          modified = true;
        }

        if (ui::itemModified()) {
          stroke_modified = true;
          selected_stroke = i;
          selected_point  = j;
        }
      }
    }

    if (!modified) {
      float base_length = 20.;
      for (int i = 0; i < strokes.size(); i++) {
        for (int j = 0; j < strokes[i].P.size(); j++) {
          // ImVec2 thetaLen, float startTheta, const ImVec2& pos, const ImVec2&
          // minThetaLen, const ImVec2& maxThetaLen, bool selected=false );
          vec p = ui::lengthHandle(
              k++,
              ImVec2(0., base_length + strokes[i].corner_radii(j)),
              0.,
              strokes[i].P[j],
              ImVec2(0., base_length + 1),
              ImVec2(ag::pi * 2, 500));
          strokes[i].corner_radii(j) = (p(1) - base_length);

          if (ui::modified()) {
            this->modified = true;
          }

          if (ui::itemModified()) {
            stroke_modified = true;
            selected_stroke = i;
            selected_point  = j;
          }
        }
      }
    }
    // edit widths here

    switch (tool) {
      case 0:
        // drag
        interact_drag();
        if (ui::modified())
          this->modified = true;
        break;
      case 1:
        // add
        interact_draw();
        break;
    }
  }

  void stroke_gui() {
    if (selected_stroke < 0 || !strokes.size()) return;

    if (strokes.size()) {
      if (!strokes.back().P.size())
        ImGui::Text("New Stroke");
      else if (ImGui::Button("New Stroke")) {
        strokes.push_back(default_stroke);
        selected_stroke = strokes.size() - 1;
      }
    }

    bool mod = this->modified;

    if (ImGui::Button("Delete Current Stroke") && strokes.size())
    {
      if (selected_stroke > -1)
      {
        deselect_all();
        strokes.erase(strokes.begin()+selected_stroke);
        glyph.unions.clear();
        this->dirty=true;
        mod=true;
      }
    }

    if (ImGui::Button("Clear Unions")) {
      glyph.unions.clear();
      this->dirty = true;
      mod         = true;
    }

    StrokeInfo* info = &strokes[selected_stroke];

    if (ImGui::CollapsingHeader("Stroke",
                                ImGuiTreeNodeFlags_AllowItemOverlap |
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
      std::vector<std::string> spine_types = {"constant",
                                              "lerp",
                                              "sin",
                                              "chisel",
                                              "stepwise"};
      //
      //        SPINE_CONSTANT=0,
      //        SPINE_LERP=1,
      //        SPINE_SIN,
      //        SPINE_CHISEL,
      //        SPINE_STEPWISE

      float contrast = 0.5;
      widths_to_contrast(&contrast, info->style.w0, info->style.w1);
      float w0          = info->style.w0;
      float w1          = info->style.w1;
      float w           = info->style.w;
      bool  start_arrow = info->style.start_cap_id > -1;
      bool  end_arrow   = info->style.end_cap_id > -1;
      bool  difference  = info->style.difference;

      // float miter_limit = info->style.miter_limit;
      float chisel_ang = info->style.chisel_ang;
      // float smoothness = info->style.smoothness;

      mod |= ImGui::StringCombo("Spine Type",
                                &info->style.spine_type,
                                spine_types);
      mod |= ImGui::SliderFloat("Chisel angle",
                                &chisel_ang,
                                0,
                                360);  // ag::pi*2);
      mod |= ImGui::SliderFloat("w", &w, 0, 200);
      mod |= ImGui::SliderFloat("contrast", &contrast, 0, 1);
      mod |= ImGui::Checkbox("difference", &difference);

      if (!info->style.periodic) {
        mod |= ImGui::Checkbox("closed", &info->P.closed);
      }
      if (!info->P.closed) {
        mod |= ImGui::Checkbox("periodic", &info->style.periodic);
      }

      if (!info->style.periodic && !info->P.closed) {
        mod |= ImGui::Checkbox("start arrow", &start_arrow);
        mod |= ImGui::Checkbox("end arrow", &end_arrow);
      }

      if (info->P.closed || info->style.periodic) {
        start_arrow = false;
        end_arrow   = false;
      }
      // mod |= ImGui::SliderFloat("smoothness", &smoothness, 0., 30.);

      contrast_to_widths(&info->style.w0, &info->style.w1, contrast);
      info->style.w            = w;
      info->style.chisel_ang   = chisel_ang;
      info->style.start_cap_id = start_arrow ? 0 : -1;
      info->style.end_cap_id   = end_arrow ? 0 : -1;
      info->style.difference   = difference;
    }

    // info->style.smoothness = smoothness;
    if (ImGui::CollapsingHeader("Stroke",
                                ImGuiTreeNodeFlags_AllowItemOverlap |
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
      mod |= ImGui::SliderFloat("head_w", &arrow_params.head_w, 0.1, 3.);
      mod |= ImGui::SliderFloat("head_l", &arrow_params.head_l, 1., 3.);
      mod |= ImGui::SliderFloat("ang_0", &arrow_params.ang_0, 90, 160.);
      mod |= ImGui::SliderFloat("ang_1", &arrow_params.ang_1, 0, 45.);
      mod |= ImGui::SliderFloat("edge_l", &arrow_params.edge_l, 0., 0.3);
      mod |= ImGui::SliderFloat("tip_w", &arrow_params.tip_w, 0., 0.3);
      mod |= ImGui::SliderFloat("side_ang", &arrow_params.side_ang, 0., 50);
    }

    this->modified = mod;
  }

  bool isModified() const { return modified || ui::isMouseDown(); }
  void gui(bool show_widgets = true) {
    this->modified = false;
    if (true)  // show_widgets)
    {
      ui::begin();
      // ui::demo();
      ImGui::SetNextWindowPos(ImVec2(0, 30), ImGuiCond_FirstUseEver);
      std::vector<std::string> tooltips = {"Select/Move points",
                                           "Add points to current stroke",
                                           "Swap layering order under mouse",
                                           "Add/Remove union under mouse"};
      tool                              = ui::toolbar("tools", "abcd", tool, false, false, tooltips);
      if (show_widgets)
        interact();
      else
        selected_point = -1;
      ui::end();
    }

    ImGui::SetNextWindowPos(ImVec2(30, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Stroke", 0);

    if (ImGui::Button("Clear")) {
      clear();
    }

    if (glyph.chunks.size() && selected_point < 0 && tool >= 2)
      interact_layering();

    stroke_gui();
    update_vertex_widths();

    ImGui::End();
  }

  void draw() {
    for (int i = 0; i < strokes.size(); i++) {
      gfx::draw(strokes[i].P);
    }
  }

  void update_widths() {
    if (!glyph.chunks.size()) return;

    for (int i = 0; i < strokes.size(); i++) {
      if (strokes[i].P.size() > 1)
        strokes[i].W = glyph.chunks[i].merged_skel.spine.widths.row(0).t();
    }

    update_vertex_widths();
  }

  void draw_chunks() {
    if (!glyph.chunks.size()) return;

    for (int i = 0; i < strokes.size(); i++) {
      gfx::color(0, 0.5, 1);
      gfx::draw(strokes[i].P);
    }
    for (int i = 0; i < glyph.chunks.size(); i++) {
      gfx::color(0.5);

      gfx::draw(glyph.chunks[i].merged_skel.shape);

      gfx::color(0.5);
      gfx::lineWidth(2.);
      gfx::draw(glyph.chunks[i].shape);
      gfx::lineWidth(1.);

      gfx::lineStipple(2.);
      gfx::color(1, 0, 0, 0.5);
      gfx::draw(strokes[i].P);
      gfx::lineStipple(0);

      //                    gfx::color(1,0,0);
      //                    for (int k = 0; k <
      //                    graff_glyphs[i].join_caps.size(); k++)
      //                    {
      //                        gfx::fill(graff_glyphs[i].join_caps[k].cap);
      //                    }

      // graff_glyphs[i].compute_unions(tracing_params.union_thresh);
    }
  }

  void clear() {
    strokes.clear();
    deselect_all();
    partitions.clear();
    z_orders = zeros<ivec>(0);
    glyph.unions.clear();
    glyph.chunks.clear();
    this->modified = true;
    // this->dirty = true;
  }

  void generate_chunks(const SkeletalParams&   skel_params,
                       const StrokeRenderData& data) {
    std::vector<StrokeInfo> strokes_schem = strokes;
    for (int i = 0; i < strokes_schem.size(); i++) {
      Polyline P = strokes_schem[i].P;
      if (schematization_params.C > 0 && P.size() > 1) {
        P = schematize_angular(
            subdivide_poly(strokes_schem[i].P),
            schematization_params.C,
            schematization_params
                .start_ang);  //, schematize_also_opposite_angle);

        Polyline simpl = cleanup_polyline(P);
        if (simpl.size() > 1) P = simpl;
        strokes_schem[i] = map_stroke_to_poly(strokes_schem[i], P);
      }
    }

    ArrowHeadGenerator arrows(arrow_params);
    glyph.chunks = chunks_from_strokes(strokes_schem,
                                       skel_params,
                                       data,
                                       {&arrows},
                                       {&arrows});
    glyph.difference_partitions.clear();
    int k = 0;
    for (int i = 0; i < strokes_schem.size(); i++) {
      int m = strokes_schem[i].P.size() - 1;
      if (strokes_schem[i].style.difference) {
        for (int j = 0; j < m; j++) {
          glyph.difference_partitions.insert(k + j);
        }
      }
      k += m;
    }
  }

  void save(const std::string& path) {
    update_vertex_widths();
    json js;
    js["z_orders"] = ivec_to_json(z_orders);
    std::vector<json> jstrokes;
    for (int i = 0; i < strokes.size(); i++)
      if (strokes[i].P.size())
        jstrokes.push_back(stroke_info_to_json(strokes[i]));
    js["strokes"] = jstrokes;
    arma::imat U  = arma::zeros<imat>(2, glyph.unions.size());
    for (int i = 0; i < glyph.unions.size(); i++) {
      U(0, i) = glyph.unions[i].first;
      U(1, i) = glyph.unions[i].second;
    }

    js["unions"] = imat_to_json(U);
    ivec Pt      = arma::ones<ivec>(partitions.size());
    for (int i = 0; i < partitions.size(); i++) Pt(i) = partitions[i];
    js["partitions"] = ivec_to_json(Pt);
    json arrow_json;
    arrow_json["head_w"]   = arrow_params.head_w;
    arrow_json["head_l"]   = arrow_params.head_l;
    arrow_json["ang_0"]    = arrow_params.ang_0;
    arrow_json["ang_1"]    = arrow_params.ang_1;
    arrow_json["edge_l"]   = arrow_params.edge_l;
    arrow_json["tip_w"]    = arrow_params.tip_w;
    arrow_json["side_ang"] = arrow_params.side_ang;
    js["arrow_params"]     = arrow_json;

    std::string jsonstr = js.dump();
    ag::string_to_file(jsonstr, path);
  }

  void load(const std::string& path) {
    clear();

    std::string jsonstr = string_from_file(path);
    json        js      = json::parse(jsonstr);
    z_orders            = json_to_ivec(js["z_orders"]);
    strokes.clear();
    for (auto jstroke : js["strokes"]) {
      StrokeInfo info = json_to_stroke_info(jstroke);
      if (info.P.size() > 1) strokes.push_back(info);
    }

    imat U = json_to_imat(js["unions"]);
    glyph.unions.clear();
    for (int i = 0; i < U.n_cols; i++) {
      glyph.unions.push_back({(int)U(0, i), (int)U(1, i)});
    }
    ivec Pt = json_to_ivec(js["partitions"]);
    partitions.clear();
    for (int i = 0; i < Pt.n_elem; i++) partitions.push_back(Pt(i));

    if (js.find("arrow_params") != js.end()) {
      json arrow_json       = js["arrow_params"];
      arrow_params.head_w   = arrow_json["head_w"];
      arrow_params.head_l   = arrow_json["head_l"];
      arrow_params.ang_0    = arrow_json["ang_0"];
      arrow_params.ang_1    = arrow_json["ang_1"];
      arrow_params.edge_l   = arrow_json["edge_l"];
      arrow_params.tip_w    = arrow_json["tip_w"];
      arrow_params.side_ang = arrow_json["side_ang"];
    }
  }
};
//

}  // namespace ag

class DemoPresetLoader {
  /**
   * Procedure:
   * - Select preset folder
   * - When in mode, add demo preset:
   *   - saves state (to preset folder)
   *   - appends entry to json file "presets.json",
   **/
 public:
  struct Entry {
    std::string name;
    std::string mode_name;
  };

  std::vector<Entry> entries;
  std::string        directory           = "./data/demo_presets";
  std::string        current_preset_name = "Untitled";
  int                current_preset_idx  = -1;

  DemoPresetLoader() {}

  std::string current_preset_path() const {
    return ag::join_path(directory, current_preset_name + ".xml");
  }

  std::string preset_path(const std::string& name) const {
    return ag::join_path(directory, name + ".xml");
  }

  std::string json_path() const {
    return ag::join_path(directory, "presets.json");
  }
  std::string current_mode_name() const {
    if (current_preset_idx == -1) return "None";
    return entries[current_preset_idx].mode_name;
  }

  void read_json() {
    if (!ag::file_exists(json_path())) {
      ag::string_to_file("{\"entries\":[]}\n", json_path());
    }

    json j        = ag::json_from_file(json_path());
    json jentries = j["entries"];
    entries.clear();
    for (auto je : jentries) {
      Entry e;
      e.name      = je["name"];
      e.mode_name = je["mode_name"];
      entries.push_back(e);
    }
  }

  void save_json() {
    std::vector<json> jentries;
    for (const Entry& e : entries) {
      json j;
      j["name"]      = e.name;
      j["mode_name"] = e.mode_name;
      jentries.push_back(j);
    }
    json res;
    res["entries"] = jentries;
    ag::json_to_file(res, json_path());
  }

  void set_directory() {
    if (!cm::openFolderDialog(directory, "Select preset directory")) {
      return;
    }
    read_json();
  }

  int gui(AppModule* module, bool is_menu) {
    std::vector<std::string> sentries;
    for (const Entry& e : entries) {
      std::string s = e.name + " (" + e.mode_name + ")";
      sentries.push_back(s);
    }

    int select = -1;
    if (is_menu) {
      if (ImGui::BeginMenu("Load Preset")) {
        if (ImGui::BeginMenu("Current demo:")) {
          for (int i = 0; i < sentries.size(); i++) {
            if (entries[i].mode_name != module->name)
              continue;
            bool toggled = false;
            if (i == current_preset_idx) {
              toggled = true;
            }
            if (ImGui::MenuItem(sentries[i].c_str(), "", toggled)) {
              current_preset_idx  = i;
              select              = current_preset_idx;
              current_preset_name = entries[select].name;
            }
          }
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("All:")) {
          for (int i = 0; i < sentries.size(); i++) {
            bool toggled = false;
            if (i == current_preset_idx) {
              toggled = true;
            }
            if (ImGui::MenuItem(sentries[i].c_str(), "", toggled)) {
              current_preset_idx  = i;
              select              = current_preset_idx;
              current_preset_name = entries[select].name;
            }
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
    } else {
      ImGui::PushID("DemoPresetLoader");

      if (ImGui::StdListBox("Demo Presets", &current_preset_idx, sentries)) {
        select              = current_preset_idx;
        current_preset_name = entries[select].name;
        std::cout << "List box preset " << select << " " << current_preset_name << std::endl;
      }
      ImGui::StringInputText("Preset name", current_preset_name);

      // Check if current name exists
      std::string btn_name        = "Save new preset";
      bool        is_name_present = false;
      for (int i = 0; i < entries.size(); i++) {
        if (entries[i].name == current_preset_name) {
          btn_name = "Update preset";
        }
      }
      if (ImGui::Button(btn_name.c_str())) {
        if (module->save_params_request(current_preset_path())) {
          if (current_preset_idx == -1 ||
              entries[current_preset_idx].name != current_preset_name) {
            entries.push_back({current_preset_name, module->name});
            save_json();
            current_preset_idx = entries.size() - 1;
          }
        } else {
          ImGui::OpenPopup("Not implemented");
        }
      }

      if (ImGui::BeginPopupModal("Not implemented")) {
        std::string s = "Mode " + module->name + " does not implement save";
        ImGui::Text(s.c_str());
        if (ImGui::Button("Ok")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
      }
      ImGui::PopID();
    }

    return select;
  }
  // void save_and_addi
};

class SimpleJsonUI {
 public:
  SimpleJsonUI() {}

  void build(ParamList* param_list, std::string parent_str = "") {
    for (int i = 0; i < param_list->getNumParams(); i++) {
      Param* p                          = param_list->getParam(i);
      params[parent_str + p->getName()] = p;
    }

    for (int i = 0; i < param_list->getNumChildren(); i++) {
      std::string child_str = parent_str + param_list->getChild(i)->getName();
      build(param_list->getChild(i), child_str + ".");
    }
  }

  void print_all_params() {
    FILE* f = fopen("./data/all_params.txt", "w");
    std::cout << "All simple UI params:" << std::endl;
    for (auto it : params) {
      std::cout << it.first << std::endl;
      fprintf(f, "%s\n", it.first.c_str());
    }
    fclose(f);
    std::cout << "---" << std::endl;
  }

  void ui(json entry) {
    for (json::iterator elem = entry.begin(); elem != entry.end(); ++elem) {
      for (json::iterator it = elem.value().begin(); it != elem.value().end();
           ++it) {
        ui_entry(it.value(), it.key());
      }
    }
  }

  void ui_entry(json entry, const std::string& key, int depth = 0) {
    if (key[0] == '_')
      return;

    if (entry.is_string()) {
      std::string txt = entry;
      if (key == "%") {
        ImGui::TextColored(ImVec4(0, 0.89, 1., 1.), txt.c_str());
        return;
      }
    }

    if (entry.is_object()) {
      if (depth > 0) ImGui::Indent();

      int flags = ImGuiTreeNodeFlags_AllowItemOverlap;
      if (depth < 1) flags |= ImGuiTreeNodeFlags_DefaultOpen;
      bool vis = ImGui::CollapsingHeader(
          key.c_str(),
          flags);  // ImGuiTreeNodeFlags_AllowItemOverlap
                   // | ImGuiTreeNodeFlags_DefaultOpen);
      if (vis) {
        ImGui::PushID(key.c_str());
        for (json::iterator it = entry.begin(); it != entry.end(); ++it)
          ui_entry(it.value(), it.key(), depth + 1);
        ImGui::PopID();
      }

      if (depth > 0) ImGui::Unindent();

    } else if (entry.is_array()) {
      if (depth > 0) ImGui::Indent();

      int flags = ImGuiTreeNodeFlags_AllowItemOverlap;
      if (depth < 1) flags |= ImGuiTreeNodeFlags_DefaultOpen;
      bool vis = ImGui::CollapsingHeader(key.c_str(), flags);

      if (vis) {
        ImGui::PushID(key.c_str());
        for (json::iterator elem = entry.begin(); elem != entry.end(); ++elem) {
          for (json::iterator it = elem.value().begin();
               it != elem.value().end();
               ++it)
            ui_entry(it.value(), it.key(), depth + 1);
        }
        ImGui::PopID();
      }

      if (depth > 0) ImGui::Unindent();
    } else {
      auto it = params.find(key);
      if (it != params.end()) imgui(it->second, entry);
    }
  }

 public:
  std::map<std::string, cm::Param*> params;
};

namespace cm {
namespace lines {

struct Buffer {
  Mesh mesh;
  Buffer(int n = 0) {
    if (n)
      mesh.reserve(n);
  }

  bool empty() const { return mesh.vertices.size() == 0; }
  void clear() {
    mesh.clear();
  }

  void segment(const arma::vec& a, const arma::vec& b, float w) {
    double ax = a(0);
    double ay = a(1);
    double bx = b(0);
    double by = b(1);
    double xx = bx - ax;
    double xy = by - ay;
    double l  = sqrt(xx * xx + xy * xy);

    if (l > 0) {
      xx = (xx / l) * w * 0.5;
      xy = (xy / l) * w * 0.5;
    } else {
      xx = w * 0.5;
      xy = 0;
    }

    double yx = -xy;
    double yy = xx;

    double p[8][3] = {
        {ax - yx - xx, ay - yy - xy},
        {ax - yx, ay - yy},

        {bx - yx, by - yy},
        {bx - yx + xx, by - yy + xy},

        {bx + yx + xx, by + yy + xy},
        {bx + yx, by + yy},

        {ax + yx, ay + yy},
        {ax + yx - xx, ay + yy - xy}};

    double uv[8][2] = {
        {0, 0},
        {0.5, 0.},
        {0.5, 0.},
        {1, 0},
        {1, 1},
        {0.5, 1.},
        {0.5, 1.},
        {0, 1}};

    int ind = mesh.vertices.size() / 3;
    for (int i = 0; i < 8; i++) {
      mesh.vertex(p[i][0], p[i][1], p[i][2]);
      mesh.uv(uv[i][0], uv[i][1]);
    }

    mesh.triangle(ind + 0, ind + 1, ind + 6);
    mesh.triangle(ind + 0, ind + 6, ind + 7);
    mesh.triangle(ind + 1, ind + 2, ind + 5);
    mesh.triangle(ind + 1, ind + 5, ind + 6);
    mesh.triangle(ind + 2, ind + 3, ind + 4);
    mesh.triangle(ind + 2, ind + 4, ind + 5);
  }

  void polyline(const ag::Polyline& P, float w) {
    auto edges = P.edges();
    for (auto e : edges) {
      segment(P[e[0]], P[e[1]], w);
    }
  }
};

bool init(const std::string& path, int n = 30000);
void release();
void begin(double w);
void segment(const vec& a, const vec& b);
void draw(const ag::Polyline& P);
void draw(const Buffer& buf);
void end();
}  // namespace lines
}  // namespace cm
