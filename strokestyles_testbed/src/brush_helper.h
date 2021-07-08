#pragma once
#include "autograff.h"
#include "colormotor.h"

namespace ag {
using namespace arma;

class BrushHelper {
 public:
  Image ball_im;
  Image drip_img;

  struct {
    float anchor_start = 0.;
    float anchor_end   = 0.;

    float trace_thickness    = 0.1;
    bool  variable_thickness = true;
    bool  split_polylines    = false;
    float aspect_ratio       = 1.;
    float sharpness          = 2.;
    float power              = 2;
    float rot                = 0;
    float size               = 1;
    float hole_multiplier    = 2.;

    float num_traces                  = 1;
    float smoothness_variation_amount = 1.;

    float min_brush_radius  = 0.25;
    float max_brush_radius  = 1.;
    float brush_sample_dist = 1.;
    float brush_lowpass     = 1.;
    float brush_base_speed  = 0.0;
    bool  variable_width    = false;
    float min_width         = 0.0;
    float randomness        = 0.;

    bool        drip_active       = false;
    float       drip_minw         = 2;
    float       drip_maxw         = 7;
    float       drip_height_ratio = 0.5;
    float       drip_thresh       = 0.5;
    float       drip_smooth_sigma = 0.1;
    float       drip_peak_thresh  = 1.;
    int         drip_num[2]       = {0, 6};
    int         drip_subd         = 400;
    float       drip_Ac           = 0.1;
    float       drip_hvariance    = 1;
    float       drip_speed        = 0.1;
    std::string ball_texture_path = "data/images/ball_1.png";

    bool re_orient = false;

    V4 color = V4(0, 0, 0, 1);
  } params;

  void load_ball_texture(const std::string& path = "") {
    bool res = true;
    if (!path.length()) {
      res = openFileDialog(params.ball_texture_path, "png");
    } else {
      res                      = true;
      params.ball_texture_path = path;
    }
    if (res) {
      params.ball_texture_path = relative_path(params.ball_texture_path);
      ball_im                  = Image(params.ball_texture_path);
    }
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

  void draw_brush_uniform(
      Image& img, const Polyline& ctr_, double animt, float r, const vec& W) {
    bool  use_width = params.variable_thickness;
    float dist      = params.brush_sample_dist;

    if (ctr_.size() < 2) return;

    mat Pw = ctr_.mat();
    if (W.n_elem == ctr_.size() && use_width)
      Pw = join_vert(Pw, W.t() * r);
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
                  float           w,
                  double          animt,
                  float           dt,
                  const vec&      W,
                  float           vbar = -1.) {
    if (ctr_.size() < 2) return;
    vec S = speed(ctr_.mat(), dt);  //*interpRatio);
    draw_brush_speed(img,
                     ctr_,
                     S,
                     animt,
                     params.min_brush_radius * w,
                     params.max_brush_radius * w,
                     W,
                     params.variable_thickness,
                     params.brush_sample_dist,
                     params.brush_lowpass,
                     params.brush_base_speed,
                     vbar);

    if (params.drip_active) {
      draw_drips(ctr_, S, animt);
    }
  }

  mat hat(const mat& x, double sharpness) {
    return erf((1 - abs(x * 2.5)) * sharpness) * 0.5 + 0.5;
  }

  double hat(double x, double sharpness) {
    return erf((1. - abs(x * 2)) * sharpness) * 0.5 + 0.5;
  }

  void brush_image(Image* brush_img,
                   int    size,
                   double theta,
                   double aspect    = -1,
                   double power     = -1,
                   double sharpness = -1,
                   double hole      = -1) {
    if (aspect < 0) aspect = params.aspect_ratio;
    if (power < 0) power = params.power;
    if (sharpness < 0) sharpness = params.sharpness;
    if (hole < 0) hole = params.hole_multiplier;

    double w = params.size;
    double h = params.size * aspect;
    // theta = params.rot-theta;

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
        if (hole > 0) h *= (1. - hat(v * hole, sharpness));
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

  vec lognormal(const vec& x_, double x0, double mu, double sigma) {
    double eps = 1e-8;
    vec    x   = x_;
    x          = x - x0;
    x          = clamp(x, eps, x.max());

    return exp(-pow(log(x) - mu, 2) / (2 * sigma * sigma)) /
           (x * sqrt(2.0 * ag::pi) * sigma);
  }

  double lognormal_onset(double mu, double sigma) {
    return exp(mu - sigma * 3);
  }

  vec lognormal_weight(const vec& t_, double t0, double Ac, double T) {
    // Lognormal interpolation between a and b
    double sigma = sqrt(-log(1.0 - Ac));
    double mu    = 3. * sigma - log((-1. + exp(6. * sigma)) / T);
    vec    t     = t_;
    t            = t - t0 + lognormal_onset(mu, sigma);
    t            = clamp(t, 1e-20, t.max());

    vec w = 0.5 * (1 + erf((log(t) - mu) / (sqrt(2) * sigma)));
    return w;
  }

  mat lognormal_stroke(const vec& a, const vec& b, int n, double Ac = 0.01) {
    vec t   = linspace(0, 1, n);
    vec phi = lognormal_weight(t, 0, Ac, 1);
    vec d   = b - a;
    mat pts = join_horiz(d(0) * phi + a(0), d(1) * phi + a(1)).t();
    return pts;
  }

  void init_drip_image() { brush_image(&drip_img, 128, 0, 1, 2, 3.3, 0); }

  void draw_drips(const Polyline& X, const vec& s_, double animt, double h = -1) {
    vec s = -(s_ / mean(s_));
    s -= s.min();
    float animIndex = animt * X.size();

    // std::cout << s.min() << " " << s.max() << std::endl;
    std::vector<int> I = maxima(s,
                                params.drip_smooth_sigma * 0.1,
                                params.drip_peak_thresh * 0.01,
                                1e-10,
                                1);

    if (h < 1e-10) {
      aa_box box = bounding_rect(X);
      h          = rect_height(box) * params.drip_height_ratio;
    }

    for (int i : I) {
      if (s[i] < params.drip_thresh) continue;

      int n = random::randint(params.drip_num[0], params.drip_num[1]);
      if (i >= animIndex)
        break;
      for (int j = 0; j < n; j++) {
        float animamt = std::max(animIndex - j, 0.0f) / X.size();
        animamt       = std::min(1.0f, animamt * params.drip_speed);
        double xo     = random::normal();
        vec    a      = X[i] + vec({1, 0}) * xo * params.drip_hvariance;
        double amt    = exp(-(xo * xo) / 2);
        amt           = std::max(amt, 0.001);
        vec b         = a + vec({0, 1}) * random::uniform(h * 0.6, h) * amt * animamt;
        mat pts =
            lognormal_stroke(a, b, params.drip_subd, params.drip_Ac);  // 0.1);

        vec S = speed(pts, 0.001);  //*interpRatio);
        draw_brush_speed(drip_img,
                         Polyline(pts, false),
                         S,
                         1000000,
                         params.drip_minw * amt,
                         params.drip_maxw * amt,
                         zeros(0),
                         false,
                         params.brush_sample_dist);
      }
    }
  }
};

}  // namespace ag
