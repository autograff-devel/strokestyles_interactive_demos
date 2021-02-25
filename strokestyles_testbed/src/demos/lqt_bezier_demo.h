#pragma once
#include "ag_lqt_helpers.h"
#include "autograff.h"
#include "colormotor.h"
#include "common.h"

using namespace cm;
using namespace ag;
using namespace arma;

class LQTBezierDemo : public Demo {
 public:
  ContourMaker ctr_maker;

  void* getData() { return &params; }

  // typedef std::tuple<arma::vec2, arma::vec2, arma::vec2, arma::vec2>
  // cubic_bezier;

  LQTParams    lqt_params;
  PolylineList Cps;  // control polygons

  struct {
    int seed = 132;

    float padding         = 100;
    float cov_orientation = 0;
    float isotropy        = 1;
    float smoothing       = 45;
    float ctrl_ratio      = 6;
    bool  show_bezier     = true;
    bool  draw_gaussians  = true;

    bool  bez_fit   = false;
    float bez_error = 1;
    float bez_subd  = 30;

    float line_width = 2.5;
  } params;

  struct {
    V4 bg     = {1., 1., 1., 1.};
    V4 traj   = {0., 0., 0., 1.};
    V4 bezier = {0.5, 0.5, 0.5, 0.5};
  } colors;

  int tool = 1;

  std::string Cps_file = "bezier_lqt_polygons.json";

  LQTBezierDemo() : Demo("LQT (fake) Bezier demo") {
    gui_params.addBool("draw Gaussians", &params.draw_gaussians);
    gui_params.addFloat("line width", &params.line_width, 0.25, 3.);
    gui_params.addFloat("padding", &params.padding, 0, 300.);
    gui_params.addBool("show original bezier", &params.show_bezier);
    gui_params.addBool("bezier fit", &params.bez_fit);
    gui_params.addFloat("bezier fir err", &params.bez_error, 0.1, 30);
    gui_params.addFloat("bezier subd", &params.bez_subd, 6, 40);

    gui_params.newChild("LQT");
    gui_params.addFloat("smoothing", &params.smoothing, 0.01, 1000.)->describe("Smoothing amount (variance)");
    gui_params.addFloat("intermediate ratio", &params.ctrl_ratio, 0.5, 10.)->describe("Intermediate control point smoothing ratio");

    gui_params.addFloat("orientation", &params.cov_orientation, -PI, PI)->describe("Orientation of covariance ellipses");
    gui_params.addFloat("isotropy", &params.isotropy, 0.01, 1.)->describe("Isotropy of covariance ellipses");

    gui_params.addSpacer();
    gui_params.addInt("order", &lqt_params.order)->describe("Linear system order (position derivative minimised)");
    gui_params.addFloat("activation sigma", &lqt_params.activation_sigma, 0.001, 1.)->describe("Tracking density (->0 interpolating)");
    gui_params.addBool("periodic", &lqt_params.periodic)->describe("Periodic trajectory");
    gui_params.addBool("stop movement", &lqt_params.stop_movement)->describe("Force movement to stop");
    ;
    gui_params.addBool("optimize initial state", &lqt_params.optimize_initial_state)->describe("Optimize also initial state");
    ;

    gui_params.addSpacer();
    gui_params.addInt("subdivision", &lqt_params.subdivision)->describe("Numer of time steps for each segment in optimization");
    ;
    gui_params.addInt("subsampling", &lqt_params.subsampling)->describe("Subsampling of each segment after optimization");
    ;
    gui_params.addFloat("max displacement", &lqt_params.max_displacement, 0.1, 100.0)->describe("Lower, smoother trajectory, equivalent to smoothing");
    gui_params.addBool("iterative", &lqt_params.iterative_when_possible)->describe("Use iterative solution if possible");
    ;
    gui_params.addFloat("force interpolation", &lqt_params.interpolation_weight, 0.0, 0.1)->describe("Tries to pass near control points");

    gui_params.newChild("Colors");
    gui_params.addColor("bg", &colors.bg);
    gui_params.addColor("traj", &colors.traj);
    gui_params.addColor("bezier", &colors.bezier);

    // can access it with params.getFloat("foo")
    //params.loadXml(this->filename() + ".xml");
    if (file_exists(Cps_file))
      Cps = json_to_polylinelist(json_from_file(Cps_file));

    load_parameters();
    //load_simple_ui();
  }

  bool init() {
    OPENBLAS_THREAD_SET
    return true;
  }

  void exit() {
    //params.saveXml(this->filename() + ".xml");
    save_parameters();
    json_to_file(polylinelist_to_json(Cps), Cps_file);
  }

  void plot_speed(const std::string& title, const arma::vec& x) {
    if (!x.size()) return;
    float* arr = new float[x.n_rows];
    for (int i = 0; i < x.n_rows; i++) arr[i] = x(i);
    //#int values_offset = 0, const char* overlay_text = NULL, float scale_min =
    // FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0), int
    // stride = sizeof(float));
    //
    ImGui::PlotLines(title.c_str(), arr, x.n_rows, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, 100));
    delete[] arr;
  }

  void fit_control_polygons() {
    mat m = rect_in_rect_transform(bounding_rect(Cps),
                                   make_rect(0, 0, appWidth(), appHeight()),
                                   params.padding);
    Cps   = affine_mul(m, Cps);
  }

  void load_svg() {
    std::string path;
    if (!openFileDialog(path, "svg")) return;
    Cps = load_svg_bezier(path);
    fit_control_polygons();
  }

  bool gui() {
    use_simple_ui = false;

    // if (ctr_maker.pts.size() > 1) {
    //   plot_speed("Speed", speed(trajectory.Y, 0.01));
    //   // plot("Spline Speed", speed(stats.res_spline, 0.01));
    // }

    if (ImGui::Button("Load SVG Beziers...")) {
      load_svg();
    }

    if (ImGui::Button("Fit to window")) {
      fit_control_polygons();
    }

    parameter_gui();

    ui::begin();
    int k = 0;
    for (int i = 0; i < Cps.size(); i++) {
      Polyline& Cp = Cps[i];
      int       n  = Cp.size();
      for (int j = 0; j < n; j++) {
        Cp[j] = ui::dragger(k++, Cp[j]);
      }

      for (int j = 0; j < n - 1; j += 3) {
        ui::line(Cp[j], Cp[j + 1], ImColor(64, 64, 64, 128));
        ui::line(Cp[j + 2], Cp[j + 3], ImColor(64, 64, 64, 128));
      }
    }
    ui::end();
    return false;
  }

  void update() {
    // Gets called every frame before render
  }

  void render() {
    float w = appWidth();
    float h = appHeight();

    ag::random::seed(params.seed);
    gfx::clear(colors.bg);

    std::vector<LQT_res> trajectories;

    for (int i = 0; i < Cps.size(); i++) {
      Polyline bez;
      bez.piecewise_bezier(Cps[i]);
      gfx::color(colors.bezier);
      gfx::draw(bez);

      LQT_res trajectory = lqt_fake_bezier(Cps[i],
                                           params.smoothing,
                                           params.ctrl_ratio,
                                           lqt_params,
                                           params.isotropy,
                                           params.cov_orientation);
      trajectories.push_back(trajectory);

      if (params.draw_gaussians && trajectory.Y.n_cols > 0) {
        gfx::draw_gaussians_2d(trajectory.Mu, trajectory.Sigma, 0.2, vec({1., 0.6, 0.0, 0.5}));
      }

      gfx::color(colors.traj);
      gfx::lineWidth(params.line_width);
      gfx::draw(trajectory.Y, false);
    }

    if (params.bez_fit) {
      std::vector<std::vector<cubic_bezier>> Bez;
      for (int i = 0; i < trajectories.size(); i++) {
        Polyline                  traj(trajectories[i].Y, false);
        std::vector<cubic_bezier> bez = fit_bezier(traj, params.bez_error);
        Bez.push_back(bez);
        Polyline btraj = sample_beziers(bez, params.bez_subd);
        gfx::color(0, 0.6, 1.);
        gfx::draw(btraj);
      }

      if (ImGui::Button("Save Bezier...")) {
        std::string pp;
        if (saveFileDialog(pp, "svg")) {
          SvgWriter svg(pp);
          svg.stroke_width(2.);
          for (const auto& bez : Bez) {
            svg.stroke(bez);
          }
          svg.save();
        }
      }
    }
  }
};
