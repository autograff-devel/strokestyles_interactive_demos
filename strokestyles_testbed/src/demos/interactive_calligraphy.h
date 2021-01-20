#pragma once
#include <boost/filesystem.hpp>

#include "../demos/demo.h"
#include "GLFW/glfw3.h"
#include "ag_alpha_shape.h"
#include "ag_color.h"
#include "ag_graff_font.h"
#include "ag_layering.h"
#include "ag_schematize.h"
#include "ag_skeletal_strokes.h"
#include "autograff.h"
#include "brush_helper.h"
#include "deps/json.hpp"
#include "deps/nanosvg/nanosvg.h"
#include "deps/tinyutf8.h"
#include "deps/utf8.h"

namespace fs = boost::filesystem;
using json   = nlohmann::json;

using namespace cm;
using namespace ag;
using namespace arma;

class InteractiveCalligraphy : public Demo {
 public:
  int tool = 0;

  struct {
    float seed = 10.;
    float w    = 40.;

    bool debug_draw          = false;
    bool rt_update           = true;
    bool transparent_save    = true;
    bool draw_stroke_polygon = false;
    int  spine_type          = 0;

    bool gui_active = true;
    bool draw_tied_gauss = false;
    float tied_cov_x = 100;
    float tied_cov_y = 400;

    //float tied_gauss_radius = 1;
  } params;

  struct {
    int         mode               = MODE_TRACE;
    std::string svg_prototype_path = "data/fleshes/testflesh.svg";
    bool        debug_widths       = false;
    int         num_strokes        = 0;
  } mode_params;

  struct {
    double time            = 0.;
    bool   active          = false;
    float  stroke_duration = 0.8;
    bool   animating       = false;
  } screen_recording;

  enum {
    MODE_TRACE    = 0,
    MODE_BRUSH    = 1,
    MODE_BALL     = 2,
    MODE_SKELETAL = 3,
  };

  // struct
  // {
  //     float anchor_start = 0.;
  //     float anchor_end = 0.;

  //     float trace_thickness = 0.1;
  //     bool variable_thickness = true;
  //     bool split_polylines = false;
  //     float aspect_ratio = 1.;
  //     float sharpness = 2.;
  //     float power = 2;
  //     float rot = 0;
  //     float size = 1;
  //     float hole_multiplier = 2.;

  //     float num_traces = 1;
  //     float smoothness_variation_amount = 1.;

  //     float min_brush_radius = 0.25;
  //     float max_brush_radius = 1.;
  //     float brush_sample_dist = 1.;
  //     float brush_lowpass = 1.;
  //     float brush_base_speed = 0.0;
  //     bool variable_width = false;
  //     float randomness = 0.;

  //     std::string ball_texture_path="data/images/ball_1.png";

  //     bool re_orient = false;

  //     V4 color = V4(0,0,0,1);
  // } brush.params;

  struct {
    V4 color = V4(1., 1., 1., 1.);
  } background_params;

  GlyphEditor               editor;
  StrokeRenderData          render_data;
  std::vector<PolylineList> skeletal_shapes;
  std::vector<GlyphInfo>    processed_glyph_infos;
  std::vector<float>        stroke_widths;
  PolylineList              handwritten_strokes;
  std::vector<vec>          vertex_widths;

  Image       brush_im;
  BrushHelper brush;
  // Image ball_im;

  Trigger<bool>            trig_save_png;
  cm::ParamModifiedTracker param_modified;

  InteractiveCalligraphy() : Demo("Interactive Calligraphy") {
    std::vector<std::string> modes = {"Trace", "Brush", "Tube"};  //"Skeletal",

    ParamList* child;

    // clang-format off
        gui_params.addFloat("random seed", &params.seed, 1, 1000)->describe("random seed");

        child = gui_params.newChild("Visualization Options")->appendOption("hidden");
        //gui_params.addBool("debug draw", &params.debug_draw)->appendOption("n");
        gui_params.addBool("draw stroke polygon", &params.draw_stroke_polygon);
        gui_params.addBool("transparent save", &params.transparent_save);
        gui_params.addBool("update every frame", &params.rt_update);
        gui_params.addBool("gui", &params.gui_active);
        gui_params.addBool("draw tied gauss", &params.draw_tied_gauss);
        gui_params.addFloat("tied cov x", &params.tied_cov_x, 0, 500.);
        gui_params.addFloat("tied cov y", &params.tied_cov_y, 0, 500.);
        
        //gui_params.addBool("draw gaussians", &global_debug.draw_gaussians);
        //gui_params.addFloat("sigma mul", &render_data.sigma_mul, 0.01, 2.);

        
        gui_params.addSelection("Stroking mode", modes, &mode_params.mode)->describe("Stroking mode");
        gui_params.addFloat("Thickness", &render_data.width_mul, 0.1, 4.)->describe("global thickness multiplier");
        param_modified << gui_params.addBool("Variable thickess", &brush.params.variable_thickness)->describe("If true, vary stroke widths");
        param_modified << gui_params.addInt("C", &editor.schematization_params.C);
    param_modified << gui_params.addFloat(
        "start_ang",
        &editor.schematization_params.start_ang,
        0,
        180);
        param_modified << gui_params.addBool("Split polylines", &brush.params.split_polylines)->describe("Splits polylines if smoothig is disabled");
        param_modified << gui_params.addFloat("Num traces", &brush.params.num_traces, 1, 100.)->describe("Number of traces");
        param_modified << gui_params.addFloat("smoothness variation", &brush.params.smoothness_variation_amount, 0, 2);
        param_modified << gui_params.addFloat("anchor_start", &brush.params.anchor_start, -0.4, 0.4);
        param_modified << gui_params.addFloat("anchor_end", &brush.params.anchor_end, -0.4, 0.4);
        param_modified << gui_params.addInt("Num Strokes", &mode_params.num_strokes)->describe("Number of strokes to visualize, 0 is all");
        child = gui_params.newChild("Brush");
        gui_params.addColor("Color", &brush.params.color);
        gui_params.addFloat("Size", &brush.params.size, 0.1, 1.)->describe("Brush relative size");
        gui_params.addFloat("Aspect ratio", &brush.params.aspect_ratio, 0.1, 1.)->describe("Brush aspect ratio");
        param_modified << gui_params.addFloat("Rotation", &brush.params.rot, 0.0, ag::pi*2)->describe("Brush rotation");
        gui_params.addFloat("Sharpness", &brush.params.sharpness, 0.1, 10.)->describe("Brush sharpness");
        gui_params.addFloat("Power", &brush.params.power, 0.1, 10.)->describe("Brush power");
        gui_params.addFloat("Hole", &brush.params.hole_multiplier, 0.0, 3.)->describe("Hole multiplier for brush image (0, no hole)");
        gui_params.addBool("Re-orient", &brush.params.re_orient);
        gui_params.addFloat("Randomness", &brush.params.randomness, 0.0, 0.9)->describe("Brush randomness");
        gui_params.addString("ball texture", &brush.params.ball_texture_path)->noGui();

        child = gui_params.newChild("Brush Trajectory");
        gui_params.addFloat("Min radius", &brush.params.min_brush_radius, 0.001, 1.)->describe("Minimum Radius");
        gui_params.addFloat("Max radius", &brush.params.max_brush_radius, 0.1, 2.)->describe("Max Brush Radius");
        gui_params.addFloat("Sample dist", &brush.params.brush_sample_dist, 0.1, 5.)->describe("Brush sample distance");
        gui_params.addFloat("Lowpass", &brush.params.brush_lowpass, 0.01, 1.)->describe("Brush speed low pass");
        gui_params.addFloat("Base speed", &brush.params.brush_base_speed, 0.0, 1)->describe("Brush base (relative) speed");
        gui_params.addBool("Vary width", &brush.params.variable_width)->describe("Use segmemntation width for brush");

        child = gui_params.newChild("Drips");

        gui_params.addBool("drips active", &brush.params.drip_active);
        
        gui_params.addFloat("drip_minw", &brush.params.drip_minw, 0.001, 20.);
        gui_params.addFloat("drip_maxw", &brush.params.drip_maxw, 0.001, 20.);
        gui_params.addFloat("drip_height_ratio", &brush.params.drip_height_ratio, 0.01, 1.);
        gui_params.addFloat("drip_thresh", &brush.params.drip_thresh, 0.001, 10.);
        gui_params.addFloat("drip_smooth_sigma", &brush.params.drip_smooth_sigma, 0.0, 1.);
        gui_params.addFloat("drip_peak_thresh", &brush.params.drip_peak_thresh, 0.01, 1.);
        gui_params.addInt("drip_num", &brush.params.drip_num[1]);
        gui_params.addFloat("drip_Ac", &brush.params.drip_Ac, 0.001, 0.5);
        gui_params.addFloat("drip_hvariance", &brush.params.drip_hvariance, 0.1, 23.0);
        gui_params.addFloat("drip_speed", &brush.params.drip_speed, 0.001, 2.);

        child = gui_params.newChild("Trajectory generation");
        param_modified << gui_params.addBool("use shinoda", &render_data.use_shinoda);
        param_modified << gui_params.addInt("spline multiplicity", &render_data.spline_multiplicity);
        param_modified << gui_params.addFloat("shindoa cov", &render_data.shinoda_cov_multiplier, 0.1, 10.);
        gui_params.addBool("rectify", &render_data.rectify);
        
        param_modified << gui_params.addFloat("isotropy", &render_data.softie_params.isotropy, 0.01, 1.)->describe("isotropy of movement primitives (precision ellipses)");
        param_modified << gui_params.addFloat("precision angle", &render_data.softie_params.precision_angle, -180, 180)->describe("orientation of movement precision ellipse");
        param_modified << gui_params.addFloat("global smoothness", &render_data.softie_params.smoothness, 0.001, 550)->describe("global smoothness of letters");
        param_modified << gui_params.addFloat("width smoothness", &render_data.softie_params.lqt.width_cov_scale, 0.001, 15)->describe("smoothness of width paramter");
        param_modified << gui_params.addBool("opt x0", &render_data.softie_params.opt_x0);

        //
//        gui_params.addFloat("width smoothing", &render_data.width_smoothing, 0.001, 1000)->describe("global smoothness of width");
        //gui_params.addFloat("smoothing amt", &render_data.style.smoothness, 0.0, 1.)->describe("influence of global smoothing on letter");
        param_modified << gui_params.addFloat("activation sigma", &render_data.softie_params.lqt.activation_sigma, 0.0, 1.)->describe("Activation time sigma (spread)");
        param_modified << gui_params.addFloat("raidus scale", &render_data.radius_scale, 10.0, 400.)->describe("Curvature radius scale for smoothing estimation");
        param_modified << gui_params.addFloat("corner cov", &render_data.corner_cov_multiplier, 0., 100.0)->describe("Smoothing for corners");
        param_modified << gui_params.addFloat("corner thresh", &render_data.corner_smoothness_thresh, 0., 30.0)->describe("Smoothness threshold for corners");
        param_modified << gui_params.addFloat("smooth power", &render_data.smooth_power, 0., 2.0)->describe("Power for radius to smoothness comp");
        // gui_params.addFloat("free end extension", &render_data.free_end_extension, -2, 4.0)->describe("Extension for terminals");
        // gui_params.addFloat("extension x", &render_data.extension_x, 0.0, 1.0)->describe("x axis extension amount");
        // gui_params.addFloat("extension y", &render_data.extension_y, 0.0, 1.0)->describe("y axis extension amount");
        
        param_modified << gui_params.addInt("period", &render_data.softie_params.lqt.period)->describe("Periodicity overlap");
        
        param_modified << gui_params.addFloat("flat angle smoothing", &render_data.softie_params.flat_angle_smooth_amt, 0.0, 50.)->describe("smoothing relative to turning angle of polyline");
        param_modified << gui_params.addFloat("flat angle multiplier", &render_data.softie_params.flat_angle_multiplier, 0.001, 3.)->describe("angle multiplier for turning angle smoothing ");
        
        param_modified << gui_params.addBool("force periodic", &render_data.force_periodic)->describe("enforce periodic smoothing");
        param_modified << gui_params.addFloat("cap smoothness", &render_data.softie_params.cap_smoothness, 0.1, 2.)->describe("determines the smoothness of caps for periodic smoothie (lower -> sharper)");
        
        gui_params.newChild(child, "Advanced")->appendOption("hidden");
        param_modified << gui_params.addInt("order", &render_data.softie_params.lqt.order);
        param_modified << gui_params.addInt("subdivision", &render_data.softie_params.subdivision);
        param_modified << gui_params.addInt("subsampling", &render_data.softie_params.subsampling);
        param_modified << gui_params.addFloat("ref spine width", &render_data.ref_spine_width, 0.1, 20.)->describe("Spine width for rectification trajectories");
        param_modified << gui_params.addFloat("rectify sleeve eps", &render_data.rectify_sleeve_eps, 0., 100);
        
        param_modified << gui_params.addFloat("end weight", &render_data.softie_params.lqt.end_weight, 0.0, 500.)->describe("weight to enforce 0 end condition (disable?)");
        param_modified << gui_params.addFloat("end cov scale", &render_data.softie_params.lqt.end_cov_scale, 0.01, 1.)->describe("scaling for end covariance of non periodic trajectories (not necesary?)");
        
        param_modified << gui_params.addFloat("force interpolation", &render_data.softie_params.lqt.force_interpolation, 0.0, 100.)->describe("Forces trajectory to go through key-points");
        param_modified << gui_params.addFloat("centripetal alpha", &render_data.softie_params.lqt.centripetal_alpha, 0.0, 1.)->describe("Centripetal parameter (0. - uniform, 0.5 - centripetal, 1. - chordal)");
        param_modified << gui_params.addFloat("max displacement", &render_data.softie_params.lqt.max_displacement, 0.0, 50.)->describe("Max displacement for smoothing");

    // clang-format on

    // gui_params.addFloat("stroke dur", &softie_params.stroke_duration, 0.01,
    // 0.4);
    std::string pppp = getExecutablePath();
    std::cout << std::endl
              << "Current working dir is: " << pppp << std::endl;
    std::cout << fs::current_path().string() << std::endl;

    child = gui_params.newChild("Background");
    gui_params.addColor("background color", &background_params.color);

    load_parameters();
    load_simple_ui();

    if (file_exists(configuration_path(this, "_glyph.json")))
      editor.load(configuration_path(this, "_glyph.json"));

    brush.load_ball_texture(brush.params.ball_texture_path);
  }

  bool init() {
    // OPENBLAS_THREAD_SET
    brush.init_drip_image();
    return true;
  }

  void exit() {
    save_parameters();
    editor.save(configuration_path(this, "_glyph.json"));
  }

  bool gui() {
    ImGui::BeginHighlightButton();

    if (ImGui::Button("clear strokes")) {
      editor.clear();
    }

    if (ImGui::Button("Load glyph svg...")) {
      editor_load_svg();
    }

    if (ImGui::Button("Load ball...")) {
      if (openFileDialog(brush.params.ball_texture_path, "png"))
        brush.load_ball_texture(brush.params.ball_texture_path);
    }

    ImGui::SameLine();

    int n = 0;
    for (int j = 0; j < editor.glyph.chunks.size(); j++) {
      for (int k = 0; k < editor.glyph.chunks[j].shape.size(); k++)
        n += editor.glyph.chunks[j].shape[k].size();
    }

    ImGui::Text("%d points", n);
    ImGui::Text("Select pencil to add stroke points");
    ImGui::EndHighlightButton();

    if (ImGui::Button("Save png...")) trig_save_png.trigger();

    if (ImGui::Button("Save Glyph...")) {
      std::string path;

      if (saveFileDialog(path, "json")) {
        gui_params.saveXml(this->filename() + ".xml");
        editor.save(path);
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Glyph...")) {
      std::string path;
      if (openFileDialog(path, "json")) {
        editor.load(path);
      }
    }

    if (ImGui::BeginMenu("Animation")) {
      if (ImGui::MenuItem("Animating", "", &screen_recording.animating)) {
      }
      if (ImGui::MenuItem("Save Stroke Animation...")) {
        std::string path;
        if (saveFileDialog(path, "mp4")) {
          gfx::beginScreenRecording(path, appWidth(), appHeight(), 30.);
          screen_recording.time   = 0.;
          screen_recording.active = true;
          // osc.start_recording();
        }
      }

      ImGui::BeginChild("Settings", ImVec2(150, 80), true);
      ImGui::SliderFloat("Stroke duration",
                         &screen_recording.stroke_duration,
                         0.1,
                         2.);
      if (ImGui::Button("reset time")) {
        screen_recording.time = 0;
      }

      // ImGui::Checkbox("Animating", &screen_recording.animating);
      ImGui::EndChild();
      ImGui::EndMenu();
    }

    parameter_gui();

    return false;
  }

  StrokeInfo polyline_to_stroke_info(const Polyline& P, float w = 100) {
    StrokeInfo info   = editor.default_stroke;
    int        m      = P.size();
    info.W            = ones(m - 1) * w;
    info.Wv           = ones(m) * w;
    info.corner_radii = ones(m) * 150;
    info.corner_flags = zeros<ivec>(m);
    if (info.Wv.n_elem)
      info.max_w = info.Wv.max();
    else
      info.max_w = 0;

    info.P = P;
    return info;
  }

  // PolylineList load_svg_poly( const std::string & f)
  // {
  //     using namespace nsvg;
  //     nsvg::NSVGimage* image;
  //     image = nsvg::nsvgParseFromFile(fpath.c_str(), "px", 72);

  //     PolylineList S;

  //     // Use...
  //     for (nsvg::NSVGshape* shape = image->shapes; shape != NULL; shape =
  //     shape->next)
  //     {
  //         for (nsvg::NSVGpath* path = shape->paths; path != NULL; path =
  //         path->next)
  //         {
  //             if (path->pts == NULL)
  //                 continue;
  //             Polyline ctr(false);
  //             float* p;
  //             for (int i = 0; i < path->npts; i += 3)
  //             {
  //                 p = &path->pts[i*2];
  //                 ctr.add(vec({p[0],p[1]}));
  //             }
  //             // endpoint
  //             ctr.add(vec({p[6],p[7]}));

  //             S.push_back(cleanup_polyline(ctr, 0.1));
  //         }
  //     }
  //     // Delete
  //     nsvgDelete(image);

  //     return S;
  // }

  void editor_load_svg() {
    std::string path;
    if (!openFileDialog(path, "svg")) return;
    PolylineList S = load_svg(path);

    editor.clear();

    int m = 0;
    for (const Polyline& P : S) {
      editor.strokes.push_back(polyline_to_stroke_info(P));
      m += P.size() - 1;
    }

    editor.z_orders = linspace<ivec>(0, m - 1, m);
    for (int i = 0; i < m; i++) editor.partitions.push_back(i);
  }

  void draw_piecewise_polyline(const Polyline& P, const arma::ivec& corners) {
    std::vector<int> I = {0};
    int              m = P.size();

    PolylineList pieces;
    pieces.push_back(Polyline(false));
    pieces.back().add(P[0]);
    for (int i = 1; i < m - 1; i++) {
      pieces.back().add(P[i]);
      if (corners[i] == 1) {
        pieces.push_back(Polyline(false));
        pieces.back().add(P[i]);
      }
    }
    pieces.back().add(P.back());

    gfx::draw(pieces);
  }

  void update() {
    // Gets called every frame before render
  }

  void draw_soft_brush(Image& img, const vec& pos, float radius) {
    img.draw(pos[0] - radius * 0.5, pos[1] - radius * 0.5, radius, radius);
  }

  void save_state(const std::string& path) {}

  void load_state(const std::string& path) {}

  double discretize(double v, double step) { return round(v / step) * step; }

  void mode_render(bool needs_update) {
    // mode_params.mode = MODE_BALL;
    // return;
    if (Keyboard::pressed(ImGuiKey_Enter)) screen_recording.time = 0;

    if (render_data.use_shinoda) brush.params.variable_thickness = false;

    if (screen_recording.active) {
      screen_recording.time += 1. / (30 * screen_recording.stroke_duration);
    } else {
      // screen_recording.time += (1./100)*phunk_params.anim_speed;
      screen_recording.time +=
          1. / (ImGui::GetIO().Framerate * screen_recording.stroke_duration);
    }

    if (screen_recording.active &&
        screen_recording.time > handwritten_strokes.size() + 2) {
      gfx::endScreenRecording();
      screen_recording.active = false;
    }

    double time = screen_recording.time;

    if (needs_update)  // param_modified.modified() || params.rt_update)
    {
      mat cov = get_constant_covariance(render_data.softie_params);
      gfx::draw_gaussian_2d({params.tied_cov_x, params.tied_cov_y}, cov, global_debug.sigma_mul, vec({1., 0.6, 0.0, 1.}));

      stroke_widths.clear();
      processed_glyph_infos.clear();
      // editor.update_widths();

      GlyphInfo info;
      // info.strokes = editor.strokes;
      // Adjust widths hack

      for (int i = 0; i < editor.strokes.size(); i++) {
        if (editor.strokes[i].P.size() < 2) continue;
        StrokeInfo    stroke = editor.strokes[i];
        SkeletalSpine spine  = build_spine(stroke, render_data);
        int           n      = spine.widths.n_cols;
        for (int j = 0; j < n; j++) stroke.Wv(j) = spine.widths.col(j)(0);
        stroke.Wv(stroke.Wv.n_elem - 1) = spine.widths.col(n - 1)(1);
        if (editor.schematization_params.C > 0 && stroke.P.size() > 1) {
          Polyline P = schematize_angular(
              subdivide_poly(stroke.P),
              editor.schematization_params.C,
              editor.schematization_params
                  .start_ang);  //, schematize_also_opposite_angle);

          Polyline simpl = cleanup_polyline(P);
          if (simpl.size() > 1) P = simpl;
          stroke = map_stroke_to_poly(stroke, P);
        }

        info.strokes.push_back(stroke);
      }

      GlyphInstance inst(info);

      if (info.strokes.size())  //&& editor.strokes[0].P.size() > 1)
        processed_glyph_infos.push_back(
            process_glyph_instance(inst, render_data, &stroke_widths));
    }

    if (true) {
      vertex_widths.clear();
      handwritten_strokes.clear();
      for (const GlyphInfo& info : processed_glyph_infos) {
        for (int j = 0; j < info.strokes.size(); j++) {
          if (mode_params.debug_widths) {
            const StrokeInfo& stroke = info.strokes[j];
            gfx::color(1, 0, 0, 0.6);
            for (int i = 0; i < stroke.P.size(); i++) {
              gfx::drawCircle(stroke.P[i], stroke.Wv(i));  // P[i](2));
            }
          }

          //gfx::color(0, 0.5, 1);
          //gfx::draw(info.strokes[j].P);

          if (brush.params.num_traces > 1 && mode_params.mode == MODE_TRACE) {
            int num_traces = brush.params.num_traces;
            for (int k = 0; k < num_traces; k++) {
              StrokeInfo stroke = info.strokes[j];
              for (int ii = 0; ii < stroke.P.size(); ii++) {
                stroke.P[ii] += random::normal(2) * stroke.Wv(ii);
                stroke.corner_radii(ii) *=
                    pow(ag::random::uniform(0.5, 2.0),
                        brush.params.smoothness_variation_amount);
              }

              mat Pw = smooth_stroke(stroke, render_data, 0, false);
              handwritten_strokes.push_back(Polyline(Pw.rows(0, 1), false));
            }
          } else {
            mat Pw = smooth_stroke(info.strokes[j], render_data, 0, true);
            handwritten_strokes.push_back(Polyline(Pw.rows(0, 1), false));
            if (Pw.n_rows > 2) {
              // hack rectify smoothness
              vec    W     = Pw.row(2).t();
              double ratio = info.strokes[j].max_w / W.max();
              W            = W * ratio;  //(info.strokes[j].Wv.max() / W.max());
              // vertex_widths.push_back(W);//Pw.row(2).t());
              vertex_widths.push_back(
                  W % (ones(W.n_elem) + random::uniform(-1., 1., W.n_elem) *
                                            brush.params.randomness));

            } else
              vertex_widths.push_back(ones(Pw.n_cols));
          }
        }
      }

      // handwritten_strokes = smooth_string(str, render_data, &stroke_widths);
    }

    brush.brush_image(&brush_im, 256, brush.params.rot);
    // mode_params.mode = MODE_TRACE;
    switch (mode_params.mode) {
      case MODE_TRACE:
        gfx::color(brush.params.color);
        if (brush.params
                .split_polylines)  // && render_data.softie_params.smoothness <
                                   // 1e-5)
        {
          for (int i = 0; i < processed_glyph_infos.size(); i++) {
            float w = (brush.params.variable_thickness)
                          ? stroke_widths[i] * 2
                          : render_data.width_mul * 100;
            gfx::lineWidth(w * brush.params.trace_thickness);
            for (int j = 0; j < processed_glyph_infos[i].strokes.size(); j++)
              draw_piecewise_polyline(
                  processed_glyph_infos[i].strokes[j].P,
                  processed_glyph_infos[i].strokes[j].corner_flags);
          }
        } else {
          for (int i = 0; i < handwritten_strokes.size(); i++) {
            float w = (brush.params.variable_thickness)
                          ? stroke_widths[i] * 2
                          : render_data.width_mul * 100;
            if (brush.params.num_traces > 1) w = 1.;
            gfx::lineWidth(w * brush.params.trace_thickness);
            gfx::draw(handwritten_strokes[i]);
          }

          if (mode_params.debug_widths) {
            for (int i = 0; i < handwritten_strokes.size(); i++) {
              gfx::color(0, 1, 0, 0.6);
              for (int j = 0; j < handwritten_strokes[i].size(); j++) {
                gfx::drawCircle(handwritten_strokes[i][j], vertex_widths[i][j]);
              }
            }
          }
        }

        gfx::lineWidth(1);
        break;
      case MODE_BRUSH: {
        gfx::color(brush.params.color);
        int n = handwritten_strokes.size();
        if (mode_params.num_strokes > 0)
          n = std::min(n, mode_params.num_strokes);

        vec   spd  = zeros(0);
        float vbar = -1;
        //                for (int i = 0; i < n; i++)
        //                    spd = join_vert(spd,
        //                    speed(handwritten_strokes[i].mat(), 0.01));
        //                if (n)
        //                    vbar = mean(spd);

        for (int i = 0; i < n; i++) {
          float w = (render_data.width_mul / brush.params.size);

          if (!brush.params.variable_thickness)
            w *= 15;
          else {
            w = 0.1;  // std::min(w, 1.5f);
          }
          //(brush.params.variable_thickness)?stroke_widths[i]*1:
          // if (brush.params.variable_width)
          //    w = render_data.width_mul*100;

          if (false)  // brush.params.re_orient)
          {
            double theta = 0.;
            int    c     = 0;
            for (int j = 0; j < handwritten_strokes[i].size() - 1; j++) {
              vec a = handwritten_strokes[i][j];
              vec b = handwritten_strokes[i][j + 1];
              if (distance(a, b) > 1e-3) {
                vec d = b - a;
                theta += atan2(d(1), d(0));
                c += 1;
              }
            }
            if (c) {
              theta = theta / c;
              brush.brush_image(&brush_im, 256, theta);
            }
          }

          double anim_t = 10000000.;
          if (screen_recording.active || screen_recording.animating)
            anim_t = screen_recording.time - i;

          brush.draw_brush(brush_im,
                           handwritten_strokes[i],
                           w,
                           anim_t,
                           0.01,
                           vertex_widths[i],
                           vbar);  //, float baseSpeed=0. )
        }
        break;
      }

      case MODE_BALL: {
        gfx::color(1, brush.params.color(3));
        int n = handwritten_strokes.size();
        if (mode_params.num_strokes > 0)
          n = std::min(n, mode_params.num_strokes);

        for (int i = 0; i < n; i++) {
          float w = (render_data.width_mul / brush.params.size);
          if (!brush.params.variable_thickness)
            w *= 15;
          else {
            w = 0.1;  //1./brush.ball_im.width();// 0.01;  // std::min(w, 1.5f);
          }

          // if (!brush.params.variable_thickness)
          //   w *= 2;
          // else
          //   w = 4;
          //(brush.params.variable_thickness)?stroke_widths[i]*1:
          // if (brush.params.variable_width)
          //    w = render_data.width_mul*100;

          double anim_t = 10000000.;
          if (screen_recording.active || screen_recording.animating)
            anim_t = screen_recording.time - i;
          //                        draw_brush_uniform( Image& img, const
          //                        Polyline& ctr_, double animt, float r, const
          //                        vec& W, bool use_width, float dist=1.)
          brush.draw_brush_uniform(brush.ball_im,
                                   handwritten_strokes[i],
                                   anim_t,
                                   w,
                                   vertex_widths[i]);
        }
        break;
      }
    }

    gfx::color(0);
    // im.draw(0,0);
  }

  void render() {
    bool        save_png = false;
    std::string png_path = "";
    if (trig_save_png.isTriggered()) {
      if (saveFileDialog(png_path, "png")) save_png = true;
    }

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

    if (params.gui_active) {
      editor.gui(params.gui_active);
    }

    if (true) {
      mode_render(param_modified.modified() || params.rt_update ||
                  editor.isModified());
    }

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
