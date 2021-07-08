#pragma once
#include "brush_helper.h"
#include "font_stylization_base.h"

class FontStylizationStroke : public FontStylizationBase {
 public:
  enum {
    MODE_TRACE    = 0,
    MODE_BRUSH    = 1,
    MODE_BALL     = 2,
    MODE_SKELETAL = 3,
  };

  struct {
    int         mode               = MODE_TRACE;
    std::string svg_prototype_path = "data/fleshes/testflesh.svg";
    bool        debug_widths       = false;
    int         num_strokes        = 0;
  } mode_params;

  struct {
    double time            = 10000000.;
    bool   active          = false;
    float  stroke_duration = 0.8;
    bool   animating       = true;
  } screen_recording;

  struct {
    bool  bez_fit          = false;
    float bez_error        = 1;
    float bez_subd         = 30;
    float width_multiplier = 1;
    float bezier_sample    = 0;
  } bezier_params;

  std::string save_svg_path = "";
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
  // } brush_params;

  PolylineList              svg_prototype;
  std::vector<PolylineList> skeletal_shapes;
  std::vector<GlyphInfo>    processed_glyph_infos;
  std::vector<float>        stroke_widths;
  PolylineList              handwritten_strokes;
  std::vector<vec>          vertex_widths;

  Image       brush_im;
  Image       white_circle_im;
  BrushHelper brush;

  // Image ball_im;

  void load_prototype(std::string path = "") {
    bool res = true;
    if (!path.length())
      res = openFileDialog(mode_params.svg_prototype_path, "svg");
    if (res) {
      mode_params.svg_prototype_path =
          relative_path(mode_params.svg_prototype_path);
      svg_prototype                       = load_svg(mode_params.svg_prototype_path);
      this->param_modified.force_modified = true;
    }
  }

  FontStylizationStroke(const std::string& name = "Stroke Font Stylization")
      : FontStylizationBase(name, "./data/presets/stylization_stroke", true) {
    std::vector<std::string> modes = {"Trace", "Brush", "Tube"};  //"Skeletal",

    // clang-format off
        ParamList* child;
        gui_params.addBool("Debug widths", &mode_params.debug_widths);
        
        param_modified << gui_params.
					addSelection("Stroking mode", modes, &mode_params.mode)
					->describe("Stroking mode");
        param_modified << gui_params.
					addFloat("Thickness", &render_data.width_mul, 0.1, 4.)
					->describe("global thickness multiplier");
        param_modified << gui_params.
					addBool("Variable thickess", &brush.params.variable_thickness)
					->describe("If true, vary stroke widths");
        param_modified << gui_params.
					addBool("Split polylines", &brush.params.split_polylines)
					->describe("Splits polylines if smoothig is disabled");
        param_modified << gui_params.
					addFloat("Num traces", &brush.params.num_traces, 1, 100.)
					->describe("Number of traces");
        param_modified << gui_params.
					addFloat("smoothness variation", &brush.params.smoothness_variation_amount, 0, 2);
        param_modified << gui_params.
					addFloat("anchor_start", &brush.params.anchor_start, -0.4, 0.4);
        param_modified << gui_params.
					addFloat("anchor_end", &brush.params.anchor_end, -0.4, 0.4);
        gui_params.addInt("Num Strokes", &mode_params.num_strokes)
					->describe("Number of strokes to visualize, 0 is all");

    gui_params.newChild("Animation");
    gui_params.addFloat("stroke duration", &screen_recording.stroke_duration, 0.2, 2);
		
		gui_params.newChild("Bezier fitting");
		gui_params.addBool("bezier fit", &bezier_params.bez_fit);
		gui_params.addFloat("bezier fir err", &bezier_params.bez_error, 0.1, 30);
		gui_params.addFloat("bezier subd", &bezier_params.bez_subd, 6, 40);
		gui_params.addFloat("bezier width multiplier", &bezier_params.width_multiplier, 1, 20);
		gui_params.addFloat("bezier pre sampling", &bezier_params.bezier_sample, 0, 20);

		child = gui_params.newChild("Brush");
        gui_params.addColor("Color", &brush.params.color);
        gui_params.addFloat("Size", &brush.params.size, 0.1, 1.)->describe("Brush relative size");
        gui_params.addFloat("Aspect ratio", &brush.params.aspect_ratio, 0.1, 1.)->describe("Brush aspect ratio");
        param_modified << gui_params.addFloat("Rotation", &brush.params.rot, 0.0, ag::pi*2)->describe("Brush rotation");
        gui_params.addFloat("Sharpness", &brush.params.sharpness, 0.1, 10.)->describe("Brush sharpness");
        gui_params.addFloat("Power", &brush.params.power, 0.1, 10.)->describe("Brush power");
        gui_params.addFloat("Hole", &brush.params.hole_multiplier, 0.0, 3.)->describe("Hole multiplier for brush image (0, no hole)");
        gui_params.addBool("Re-orient", &brush.params.re_orient);
        param_modified << gui_params.addFloat("Randomness", &brush.params.randomness, 0.0, 0.9)->describe("Brush randomness");
        gui_params.addString("ball texture", &brush.params.ball_texture_path)->noGui();

        child = gui_params.newChild("Brush Trajectory");
        gui_params.addFloat("Min radius", &brush.params.min_brush_radius, 0.001, 1.)->describe("Minimum Radius");
        gui_params.addFloat("Max radius", &brush.params.max_brush_radius, 0.1, 2.)->describe("Max Brush Radius");
        gui_params.addFloat("Sample dist", &brush.params.brush_sample_dist, 0.1, 5.)->describe("Brush sample distance");
        gui_params.addFloat("Lowpass", &brush.params.brush_lowpass, 0.01, 1.)->describe("Brush speed low pass");
        gui_params.addFloat("Base speed", &brush.params.brush_base_speed, 0.0, 1)->describe("Brush base (relative) speed");
        gui_params.addBool("Vary width", &brush.params.variable_width)->describe("Use segmemntation width for brush");
				param_modified << gui_params.addFloat("Min width", &brush.params.min_width, 0., 10.)->describe("Minimum brush width");

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

        child = gui_params.newChild("Trajectory generation");
        param_modified << gui_params.addBool("rectify", &render_data.rectify);
        
        param_modified << gui_params.
					addFloat("isotropy", &render_data.softie_params.isotropy, 0.1, 1.)
					->describe("isotropy of movement primitives (precision ellipses)");
        param_modified << gui_params.
					addFloat("precision angle", &render_data.softie_params.precision_angle, -180, 180)
					->describe("orientation of movement precision ellipse");
        param_modified << gui_params.
					addFloat("global smoothness", &render_data.softie_params.smoothness, 0.001, 50)
					->describe("global smoothness of letters");
        param_modified << gui_params.
					addFloat("width smoothness", &render_data.softie_params.lqt.width_cov_scale, 0.001, 15)
					->describe("smoothness of width paramter");
        param_modified << gui_params.
					addBool("opt x0", &render_data.softie_params.opt_x0)
			->describe("Optimizes for the initial trajectory state (slower, alike starting movement from air)");
		
//        param_modified << gui_params.addFloat("width smoothing", &render_data.width_smoothing, 0.001, 1000)->describe("global smoothness of width");
        //param_modified << gui_params.addFloat("smoothing amt", &render_data.style.smoothness, 0.0, 1.)->describe("influence of global smoothing on letter");
        param_modified << gui_params.addFloat("activation sigma", &render_data.softie_params.lqt.activation_sigma, 0.0, 1.)->describe("Activation time sigma (spread)");
        param_modified << gui_params.addFloat("raidus scale", &render_data.radius_scale, 10.0, 400.)->describe("Curvature radius scale for smoothing estimation");
        param_modified << gui_params.addFloat("corner cov", &render_data.corner_cov_multiplier, 0., 0.1)->describe("Smoothing for corners");
        param_modified << gui_params.addFloat("corner thresh", &render_data.corner_smoothness_thresh, 0., 1.0)->describe("Smoothness threshold for corners");
        param_modified << gui_params.addFloat("smooth power", &render_data.smooth_power, 0., 2.0)->describe("Power for radius to smoothness comp");
        param_modified << gui_params.addFloat("miter limit", &skel_params.miter_limit, 0.09, 4.);
        // param_modified << gui_params.addFloat("free end extension", &render_data.free_end_extension, -2, 4.0)->describe("Extension for terminals");
        // param_modified << gui_params.addFloat("extension x", &render_data.extension_x, 0.0, 1.0)->describe("x axis extension amount");
        // param_modified << gui_params.addFloat("extension y", &render_data.extension_y, 0.0, 1.0)->describe("y axis extension amount");
        
        param_modified << gui_params.addBool("unfold", &skel_params.unfold);
        //gui_params.addBool("stepwise", &params.stepwise)->describe("if toggled, use random widths between w0 and w1 for each segment\notherwise use fixed width");
        param_modified << gui_params.addFloat("fold angle sigma", &skel_params.fold_angle_sigma, 0.01, ag::pi);
        
        param_modified << gui_params.addInt("period", &render_data.softie_params.lqt.period)->describe("Periodicity overlap");
        
        param_modified << gui_params.addFloat("flat angle smoothing", &render_data.softie_params.flat_angle_smooth_amt, 0.0, 50.)->describe("smoothing relative to turning angle of polyline");
        param_modified << gui_params.addFloat("flat angle multiplier", &render_data.softie_params.flat_angle_multiplier, 0.001, 3.)->describe("angle multiplier for turning angle smoothing ");
        
        param_modified << gui_params.addBool("force periodic", &render_data.force_periodic)->describe("enforce periodic smoothing");
        param_modified << gui_params.addFloat("cap smoothness", &render_data.softie_params.cap_smoothness, 0.1, 2.)->describe("determines the smoothness of caps for periodic smoothie (lower -> sharper)");
        
        gui_params.newChild(child, "Advanced")->appendOption("hidden");
        param_modified << gui_params.addInt("order", &render_data.softie_params.lqt.order);
        param_modified << gui_params.addInt("subdivision", &render_data.softie_params.subdivision);
        param_modified << gui_params.addInt("subsampling", &render_data.softie_params.subsampling);
        //param_modified << gui_params.addInt("ref subdivision", &render_data.ref_subdivision);
        param_modified << gui_params.addFloat("ref spine width", &render_data.ref_spine_width, 0.1, 20.)->describe("Spine width for rectification trajectories");
        param_modified << gui_params.addFloat("rectify sleeve eps", &render_data.rectify_sleeve_eps, 0., 100);
        
        param_modified << gui_params.addFloat("end weight", &render_data.softie_params.lqt.end_weight, 0.0, 500.)->describe("weight to enforce 0 end condition (disable?)");
        param_modified << gui_params.addFloat("end cov scale", &render_data.softie_params.lqt.end_cov_scale, 0.01, 1.)->describe("scaling for end covariance of non periodic trajectories (not necesary?)");
        
        param_modified << gui_params.addFloat("force interpolation", &render_data.softie_params.lqt.force_interpolation, 0.0, 100.)->describe("Forces trajectory to go through key-points");
        param_modified << gui_params.addFloat("centripetal alpha", &render_data.softie_params.lqt.centripetal_alpha, 0.0, 1.)->describe("Centripetal parameter (0. - uniform, 0.5 - centripetal, 1. - chordal)");
        param_modified << gui_params.addFloat("max displacement", &render_data.softie_params.lqt.max_displacement, 0.0, 10.)->describe("Max displacement for smoothing");

    // clang-format on

    // Finalize
    load_data();

    brush.load_ball_texture(brush.params.ball_texture_path);
    white_circle_im = Image("data/images/circle.png");
    // load_ball_texture(brush.params.ball_texture_path);
  }

  void mode_init() { brush.init_drip_image(); }

  void mode_menu() {
    if (ImGui::MenuItem("Save Illustrator SVG...")) {
      save_svg_path = "";
      if (saveFileDialog(save_svg_path, "svg")) {
      }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Load Tube Texture...")) {
      brush.load_ball_texture();
    }

    if (ImGui::BeginMenu("Animation")) {
      // if (ImGui::MenuItem("Animating", "", &screen_recording.animating)) {
      // }
      if (ImGui::Checkbox("Animating", &screen_recording.animating)) {
      }
      ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1.), "Press ENTER to start anim");
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
  }

  void mode_gui() {
    // if(ImGui::Button("Load prototype..."))
    //     load_prototype();

    // if (true) {  // mode_params.mode == MODE_TRACE) {
    //   if (ImGui::Button("Save For Illustrator")) {
    //     save_svg_path = "";
    //     if (saveFileDialog(save_svg_path, "svg")) {
    //     }
    //   }
    // }
  }

  PolylineList masked_prototype(const PolylineList& shape,
                                double              t)  // double stepx, double stepy)
  {
    aa_box       rect = bounding_rect(shape);
    Polyline     mask = rect_polyline(scale_rect(rect, t, 1., -1, 0));
    PolylineList res;
    for (int i = 0; i < shape.size(); i++) {
      PolylineList clipped = shape_intersection(shape[i], mask);
      if (clipped.size())
        append(&res, clipped);
      else
        res.push_back(Polyline(true));
    }
    // return shape_intersection(shape, mask);
    return res;
  }

  void draw_brush_speed(Image&          img,
                        const Polyline& ctr_,
                        arma::vec       S,
                        double          animt,
                        float           rMin,
                        float           rMax,
                        const vec&      W,
                        bool            use_width,
                        float           dist      = 1.,
                        float           lowpass   = 1.,
                        float           baseSpeed = 0.,
                        float           vbar      = -1.) {
    if (ctr_.size() < 2) return;

    float l = chord_length(ctr_);
    if (std::isnan(l)) return;

    vec X = linspace(0, 1, l / dist);
    // std::cout << arma::max(S) << std::endl;
    if (vbar <= 0) vbar = arma::max(S);

    S = arma::clamp(S, max(S) * baseSpeed, max(S));

    S = exp(-(S + vbar) / (vbar + 1e-100));

    S     = interpolate(S, X);
    mat P = ctr_.mat();
    if (use_width && W.n_elem == ctr_.size()) {
      P = join_vert(P, W.t());
      // W.print();
    } else
      use_width = false;
    Polyline ctr(interpolate(P, X, ctr_.closed), ctr_.closed);

    // Make sure animation index is adjusted
    float animIndex = clamp(animt, 0., 1.) * ctr_.size();

    float interpRatio = (float)ctr.size() / ctr_.size();
    animIndex         = interpRatio * animIndex;

    img.bind();
    int n = std::min(ctr.size(), (int)animIndex);

    float w = 0.0;

    for (int i = 0; i < n; i++) {
      const vec& p     = ctr[i];
      double     x     = p(0);
      double     y     = p(1);
      double     width = S[i];
      if (use_width) width = p(2);
      float w2 = rMin + (rMax - rMin) * width;  // S[i]*width;
      w *= width;
      w += (w2 - w) * lowpass;
      img.draw(x - w, y - w, w * 2, w * 2);
    }
    img.unbind();
  }

  void draw_brush_uniform(Image&          img,
                          const Polyline& ctr_,
                          double          animt,
                          float           r,
                          const vec&      W,
                          bool            use_width,
                          float           dist = 1.) {
    if (ctr_.size() < 2) return;

    mat Pw = ctr_.mat();
    if (W.n_elem == ctr_.size() && use_width)
      Pw = join_vert(Pw, W.t());
    else
      Pw = join_vert(Pw, ones(Pw.n_cols).t() * r);

    Pw = uniform_sample(Pw, dist);

    int n = Pw.n_cols;
    // Make sure animation index is adjusted
    float animIndex = clamp(animt, 0., 1.) * n;

    img.bind();
    n = std::min(n, (int)animIndex);

    float w = 0.0;

    for (int i = 0; i < n; i++) {
      const vec& p = Pw.col(i);
      double     w = p(2);
      double     x = p(0);
      double     y = p(1);
      img.draw(x - w, y - w, w * 2, w * 2);
    }
    img.unbind();
  }

  void draw_brush(Image&          img,
                  const Polyline& ctr_,
                  double          animt,
                  float           dt,
                  float           rMin,
                  float           rMax,
                  const vec&      W,
                  bool            use_width,
                  float           dist      = 1.,
                  float           lowpass   = 1.,
                  float           baseSpeed = 0.,
                  float           vbar      = -1.) {
    if (ctr_.size() < 2) return;
    vec S = speed(ctr_.mat(), dt);  //*interpRatio);
    draw_brush_speed(img,
                     ctr_,
                     S,
                     animt,
                     rMin,
                     rMax,
                     W,
                     use_width,
                     dist,
                     lowpass,
                     baseSpeed,
                     vbar);
  }

  mat hat(const mat& x, double sharpness) {
    return erf((1 - abs(x * 2.5)) * sharpness) * 0.5 + 0.5;
  }

  double hat(double x, double sharpness) {
    return erf((1. - abs(x * 2)) * sharpness) * 0.5 + 0.5;
  }

  void brush_image(Image* brush_img,
                   double w,
                   double h,
                   int    size,
                   double theta,
                   double power,
                   double sharpness) {
    size          = size - 1;
    w             = w * size;
    h             = h * size;
    double radius = std::max(w, h);
    vec    x      = linspace((-size / 2) / radius, (size / 2) / radius, size + 1);
    vec    y      = linspace((-size / 2) / radius, (size / 2) / radius, size + 1);
    // rotation
    double ct  = cos(theta);
    double st  = sin(theta);
    mat    img = zeros(size + 1, size + 1);
    for (int i = 0; i <= size; i++)
      for (int j = 0; j <= size; j++) {
        // transform
        double xp = (x(j) * ct - y(i) * st) / (w / radius);
        double yp = (x(j) * st + y(i) * ct) / (h / radius);
        // superellipse distance
        double v = sqrt(pow(fabs(xp), power) + pow(fabs(yp), power));
        double h = hat(v, sharpness);  //
        if (brush.params.hole_multiplier > 0)
          h *= (1. - hat(v * brush.params.hole_multiplier, sharpness));
        img(i, j) = clamp(h, 0., 1.);
      }
    // img = sqrt(pow(abs(x),power) + pow(abs(y),power));
    uchar_mat  im      = arma::conv_to<uchar_mat>::from(img * 255);
    uchar_cube im_bgra = zeros<uchar_cube>(im.n_rows, im.n_cols, 4);
    im_bgra.slice(3)   = im;
    brush_img->update(im_bgra);
    brush_img->updateTexture();
    // return imImage(im_bgra);
  }

  PolylineList stroke_pieces(const Polyline& P, const arma::ivec& corners) {
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
    return pieces;
  }

  void draw_piecewise_polyline(const Polyline& P, const arma::ivec& corners) {
    auto pieces = stroke_pieces(P, corners);
    gfx::draw(pieces);
  }

  void construct_skeletal_strokes() {
    skeletal_shapes.clear();

    for (int i = 0; i < handwritten_strokes.size(); i++) {
      // arma::mat P = handwritten_strokes[i];
      Polyline     P     = cleanup_polyline(handwritten_strokes[i], 1., 0);
      PolylineList proto = svg_prototype;
      aa_box       rect  = bounding_rect(proto);
      aa_box       rectl = scale_rect(rect, 1. + brush.params.anchor_start, 1., 1, 0);
      aa_box       rectr = scale_rect(rect, 1. + brush.params.anchor_end, 1., -1, 0);
      rect[0]            = rectl[0];
      rect[1]            = rectr[1];

      float w = (brush.params.variable_thickness)
                    ? stroke_widths[i] / rect_height(rect)
                    : render_data.width_mul;
      SkeletalSpine spine = SkeletalSpine::constant(P, w);

      double x = 1.;
      // if (screen_recording.active)
      //    x = screen_recording.time - i;

      x = std::min(x, 1.);
      if (x > 0.) {
        float        ratio       = 1.;  // 10. / rect_height(rect);
        PolylineList scaledproto = proto;
        if (x < 1.)
          scaledproto =
              masked_prototype(affine_mul(scaling_2d(ratio, ratio), proto), x);

        for (int j = 0; j < scaledproto.size(); j++)
          if (scaledproto[j].is_corrupt()) std::cout << "corrupt prototype\n";

        SkeletalParams pars;
        pars.unfold      = true;
        pars.miter_limit = 2.;
        if (spine.points.is_corrupt()) std::cout << "Corrupt spine\n";
        SkeletalStroke stroke(spine, scaledproto, rect, pars);

        for (int j = 0; j < stroke.shape.size(); j++) {
          if (stroke.shape[j].is_corrupt()) std::cout << "Corrupt stroke\n";
          PolylineList S = shape_union(stroke.shape[j], stroke.shape[j]);
          skeletal_shapes.push_back(S);
        }
      }
    }
  }

  void mode_render(bool needs_update) {
    // mode_params.mode = MODE_BALL;
    // return;
    // make sure subdivision is always > 1
    if (render_data.softie_params.subdivision <= 2)
      render_data.softie_params.subdivision = 2;

    if (Keyboard::pressed(ImGuiKey_Enter)) screen_recording.time = 0;

    if (needs_update)
      screen_recording.time = 10000000.;

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

    if (needs_update || param_modified.modified() || params.rt_update) {
      stroke_widths.clear();
      processed_glyph_infos.clear();

      for (int i = 0; i < str.size(); i++) {
        //gfx::pushMatrix(str[i].tm);
        processed_glyph_infos.push_back(
            process_glyph_instance(str[i], render_data, &stroke_widths));
        //gfx::popMatrix();
      }
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
              W            = brush.params.min_width * 10 + W * ratio;  //(info.strokes[j].Wv.max() / W.max());
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

      if (mode_params.mode == MODE_SKELETAL) {
        construct_skeletal_strokes();
      }
    }

    std::vector<Polyline>                  bezier_samples;
    std::vector<std::vector<cubic_bezier>> bezier_strokes;

    int j = 0;
    if (bezier_params.bez_fit) {
      for (int i = 0; i < processed_glyph_infos.size(); i++) {
        for (int j = 0; j < processed_glyph_infos[i].strokes.size(); j++) {
          PolylineList pieces =
              stroke_pieces(processed_glyph_infos[i].strokes[j].P,
                            processed_glyph_infos[i].strokes[j].corner_flags);
          std::vector<cubic_bezier> bez;

          for (const Polyline& P : pieces) {
            Polyline Ps;
            if (bezier_params.bezier_sample > 0)
              Ps = uniform_sample(P, bezier_params.bezier_sample);
            else
              Ps = P;
            std::vector<cubic_bezier> b =
                fit_bezier(Ps, bezier_params.bez_error);
            append(&bez, b);
          }

          Polyline btraj = sample_beziers(bez, bezier_params.bez_subd);
          bezier_samples.push_back(btraj);
          bezier_strokes.push_back(bez);
        }
      }
    }

    brush.brush_image(&brush_im, 256, brush.params.rot);
    // brush_image(&brush_im, brush.params.size,
    // brush.params.size*brush.params.aspect_ratio, 256, brush.params.rot,
    // brush.params.power, brush.params.sharpness); mode_params.mode =
    // MODE_TRACE;
    switch (mode_params.mode) {
      case MODE_SKELETAL:
        for (int i = 0; i < skeletal_shapes.size(); i++) {
          gfx::color(brush.params.color);
          gfx::fill(skeletal_shapes[i]);
        }
        break;
      case MODE_TRACE:
        gfx::color(brush.params.color);
        for (int i = 0; i < bezier_samples.size(); i++) {
          // std::cout << "Stroke width: " << stroke_widths[i] << std::endl;
          brush.draw_brush_uniform(
              white_circle_im,
              bezier_samples[i],
              10000,
              stroke_widths[i] * bezier_params.width_multiplier,
              zeros(0));
        }

        // if (brush.params
        //         .split_polylines)  // && render_data.softie_params.smoothness
        //         <
        //                            // 1e-5)
        // {
        //   for (int i = 0; i < processed_glyph_infos.size(); i++) {
        //     float w = (brush.params.variable_thickness)
        //                   ? stroke_widths[i] * 2
        //                   : render_data.width_mul * 100;
        //     gfx::lineWidth(w * bezier_params.width_multiplier);
        //     for (int j = 0; j < processed_glyph_infos[i].strokes.size(); j++)
        //       draw_piecewise_polyline(
        //           processed_glyph_infos[i].strokes[j].P,
        //           processed_glyph_infos[i].strokes[j].corner_flags);
        //   }
        // } else {
        //   for (int i = 0; i < handwritten_strokes.size(); i++) {
        //     float w = (brush.params.variable_thickness)
        //                   ? stroke_widths[i] * 2
        //                   : render_data.width_mul * 100;
        //     if (brush.params.num_traces > 1) w = 1.;
        //     gfx::lineWidth(w * bezier_params.width_multiplier);
        //     gfx::draw(handwritten_strokes[i]);
        //   }

        //   if (mode_params.debug_widths) {
        //     for (int i = 0; i < handwritten_strokes.size(); i++) {
        //       gfx::color(0, 1, 0, 0.6);
        //       for (int j = 0; j < handwritten_strokes[i].size(); j++) {
        //         gfx::drawCircle(handwritten_strokes[i][j],
        //         vertex_widths[i][j]);
        //       }
        //     }
        //   }
        // }

        gfx::lineWidth(params.chunk_line_width);

        gfx::color(1, 0, 0);
        for (int i = 0; i < bezier_samples.size(); i++) {
          gfx::draw(bezier_samples[i]);
        }
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
            w = 4.;  // std::min(w, 1.5f);
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
              // theta = theta/c;
              // brush_image(&brush_im, brush.params.size,
              // brush.params.size*brush.params.aspect_ratio, 256,
              // brush.params.rot-theta, brush.params.power,
              // brush.params.sharpness);
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
            w *= 2;
          else
            w = 4;
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

    if (save_svg_path != "") {
      std::cout << "Saving " << save_svg_path << std::endl;
      SvgWriter svg(appWidth(), appHeight(), save_svg_path);
      svg.color(0, 0, 0);
      for (int i = 0; i < bezier_strokes.size(); i++) {
        float w = stroke_widths[i];
        svg.stroke_width(
            w * 2 *
            bezier_params
                .width_multiplier);  // * brush.params.trace_thickness);
        svg.stroke(bezier_strokes[i]);
      }
      svg.save();
      save_svg_path = "";
    }

    gfx::color(0);
    // im.draw(0,0);
  }
};
