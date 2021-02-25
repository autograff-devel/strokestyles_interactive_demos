#pragma once
#include "autograff.h"
#include "colormotor.h"
#include "common.h"
#include "ag_lqt_helpers.h"

using namespace cm;
using namespace ag;
using namespace arma;

class LQTSemitiedDemo : public Demo {
 public:
  ContourMaker ctr_maker;

  void* getData() { return &params; }

  // typedef std::tuple<arma::vec2, arma::vec2, arma::vec2, arma::vec2>
  // cubic_bezier;

  LQTParams lqt_params;
  LQT_res   trajectory;

  struct {
    int seed = 132;

    float cov_orientation = 0;
    float isotropy        = 1;
    float smoothing       = 45;

    bool draw_gaussians          = false;
    bool show_osculating_circles = false;

    bool  bez_fit   = false;
    float bez_error = 1;
    float bez_subd  = 30;

    float line_width = 2.5;
  } params;

  struct {
    V4 bg         = {1., 1., 1., 1.};
    V4 traj       = {0., 0., 0., 1.};
    V4 osculating = {0.5, 0.5, 0.5, 0.3};
  } colors;

  int tool = 1;

  LQTSemitiedDemo() : Demo("LQT Semitied demo") {
    gui_params.addBool("draw Gaussians", &params.draw_gaussians);
    gui_params.addBool("show osculating circles", &params.show_osculating_circles);
  gui_params.addFloat("line width", &params.line_width, 0.25, 3.);
    gui_params.addBool("bezier fit", &params.bez_fit);
    gui_params.addFloat("bezier fir err", &params.bez_error, 0.1, 30);
    gui_params.addFloat("bezier subd", &params.bez_subd, 6, 40);

    gui_params.newChild("LQT");
    gui_params.addFloat("smoothing", &params.smoothing, 0.01, 1000.)->describe("Smoothing amount (variance)");;
    gui_params.addFloat("orientation", &params.cov_orientation, -PI, PI)->describe("Orientation of covariance ellipses");
    gui_params.addFloat("isotropy", &params.isotropy, 0.01, 1.)->describe("Isotropy of covariance ellipses");;

    gui_params.addSpacer();
    gui_params.addInt("order", &lqt_params.order)->describe("Linear system order (position derivative minimised)");
    gui_params.addFloat("activation sigma", &lqt_params.activation_sigma, 0.001, 1.)->describe("Tracking density (->0 interpolating)");
    gui_params.addBool("periodic", &lqt_params.periodic)->describe("Periodic trajectory");
    gui_params.addBool("stop movement", &lqt_params.stop_movement)->describe("Force movement to stop");;
    gui_params.addBool("optimize initial state", &lqt_params.optimize_initial_state)->describe("Optimize also initial state");;
    
    gui_params.addSpacer();
    gui_params.addInt("subdivision", &lqt_params.subdivision)->describe("Numer of time steps for each segment in optimization");;
    gui_params.addInt("subsampling", &lqt_params.subsampling)->describe("Subsampling of each segment after optimization");;
    gui_params.addFloat("max displacement", &lqt_params.max_displacement, 0.1, 100.0)->describe("Lower, smoother trajectory, equivalent to smoothing");
    gui_params.addBool("iterative", &lqt_params.iterative_when_possible)->describe("Use iterative solution if possible");;
    gui_params.addFloat("force interpolation", &lqt_params.interpolation_weight, 0.0, 0.1)->describe("Tries to pass near control points");

    gui_params.newChild("Colors");
    gui_params.addColor("bg", &colors.bg);
    gui_params.addColor("traj", &colors.traj);
    gui_params.addColor("osculating", &colors.osculating);

    // can access it with params.getFloat("foo")
    //params.loadXml(this->filename() + ".xml");
    
    ctr_maker.load(this->filename() + "_ctr.json");
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
    ctr_maker.save(this->filename() + "_ctr.json");
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

  bool gui() {
    use_simple_ui = false;

    if (ImGui::Button("Clear")) {
      ctr_maker.clear();
    }
    
    if (ctr_maker.pts.size() > 1 && trajectory.Y.n_cols > 2) {
      plot_speed("Speed", speed(trajectory.Y, 0.01));
      // plot("Spline Speed", speed(stats.res_spline, 0.01));
    }

    parameter_gui();

    ui::begin();
    // ui::demo();
    tool = ui::toolbar("tools", "ab", tool);
    ctr_maker.interact(tool);
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

    if (ctr_maker.pts.size() < 2)
      return;

    trajectory = lqt_trajectory_semitied(ctr_maker.pts,
                                         params.smoothing,
                                         params.isotropy,
                                         params.cov_orientation,
                                         lqt_params);

    if (params.draw_gaussians && trajectory.Y.n_cols > 0)  {
      gfx::color(0, 0.5);
      gfx::lineWidth(0.5);
      gfx::lineStipple(2);
      ctr_maker.draw();
      gfx::lineStipple(0);
      gfx::lineWidth(1.);

      gfx::draw_gaussians_2d(trajectory.Mu, trajectory.Sigma, 1., vec({1., 0.6, 0.0, 0.5}));
    }

    mat state = trajectory.X;

    if (params.show_osculating_circles) {
      gfx::lineWidth(0.25);
      // add acceleration if second order
      if (lqt_params.order == 2) {
        state = join_vert(state, trajectory.U);
      }

      mat ev = evolute(state, true);
      mat xy = ev.rows(0, 1);
      mat R  = ev.row(2);

      if (params.show_osculating_circles) {
        gfx::color(colors.osculating);
        for (int i = 0; i < state.n_cols; i++) {
          if (R(i) < 2000) gfx::drawCircle(xy.col(i), R(i));
        }
      }
      gfx::lineWidth(1.);
    }

    gfx::color(colors.traj);
    gfx::lineWidth(params.line_width);
    gfx::draw(trajectory.Y, false);

    if (params.bez_fit) {
      Polyline                  traj(trajectory.Y, false);
      std::vector<cubic_bezier> bez   = fit_bezier(traj, params.bez_error);
      Polyline                  btraj = sample_beziers(bez, params.bez_subd);
      gfx::color(0, 0.6, 1.);
      gfx::draw(btraj);

      if (ImGui::Button("Save Bezier...")) {
        std::string pp;
        if (saveFileDialog(pp, "svg")) {
          SvgWriter svg(pp);
          svg.stroke_width(2.);
          svg.stroke(bez);
          svg.save();
        }
      }
    }
  }
};
