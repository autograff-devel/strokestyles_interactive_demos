#pragma once
#include <boost/filesystem.hpp>

#include "GLFW/glfw3.h"
#include "ag_alpha_shape.h"
#include "ag_color.h"
#include "ag_graff_font.h"
#include "ag_layering.h"
#include "ag_schematize.h"
#include "ag_skeletal_strokes.h"
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

struct Interval {
  double t          = 0;
  float  tempo      = 1.;
  float  half_proba = 0;
  bool   half       = false;
  bool   active     = true;

  Interval() { t = 0; }

  bool tick(double dt) {
    t += dt;
    if (t > tempo) {
      while (t > tempo) t -= tempo;
      return true;
    }
    return false;
  }
};

struct Particle {
  Particle(int id) { this->id = id; }

  void reset() {
    pos   = {0., 0.};
    vel   = {0., 0.};
    accel = {0., 0.};
    rand  = random::uniform(0, 1);
    t     = 0;
  }

  vec    pos;
  vec    vel;
  vec    accel;
  double rand     = 0;
  double rand2    = 0;
  double lifetime = 5.;
  double t        = 0;
  int    id;
};

class Force {
 public:
  virtual vec  get(Particle* p) { return {0., 0.}; }
  virtual void update(double dt) {}
};

class Particles {
 public:
  // std::vector<Force*> forces;
  std::set<int>         active;
  std::vector<Particle> pool;
  std::vector<int>      inactive;

  Particles(int pool_size) {
    pool.reserve(pool_size);
    inactive.reserve(pool_size);
    for (int i = 0; i < pool_size; i++) {
      pool.push_back(Particle(i));
      inactive.push_back(i);
    }
  }

  Particle* spawn(const arma::vec& pos, double lifetime = 5.) {
    if (!inactive.size()) return 0;

    int id = inactive.back();
    inactive.pop_back();

    active.insert(id);
    Particle* p = &pool[id];
    p->reset();
    p->pos      = pos;
    p->rand     = random::uniform(0, 1);
    p->rand2    = 1;  /// random::uniform(-1,1);
    p->lifetime = lifetime;
    return p;
  }

  void kill(Particle* p) {
    inactive.push_back(p->id);
    active.erase(p->id);
  }

  void update(double dt, Force* force = 0) {
    if (force) force->update(dt);
    //std::cout << "Active particles: " << active.size() << std::endl;
    std::vector<Particle*> killed;
    for (int id : active) {
      Particle* p     = &pool[id];
      vec       accel = p->accel;  // centripetal_force(p.t*np.pi*10+p.rand*np.pi*2,
                                   // 56000*p.rand)
      if (force) accel += force->get(p);

      p->vel += accel * dt;
      p->pos += p->vel * dt;
      // p.accel *= (1. - damping*dt)
      p->t += dt;

      if (p->t > p->lifetime) killed.push_back(p);
    }

    for (Particle* p : killed) kill(p);
  }

  void render(double           size,
              const arma::vec& color,
              Image*           img     = 0,
              ASEPalette*      palette = 0) {
    gfx::color(color);
    if (img) {
      img->bind(0);
      gfx::enablePointSprites(true, 0);
      gfx::setPointSize(size);
      gfx::beginVertices(gfx::POINTS);
      for (int id : active) {
        Particle* p = &pool[id];
        if (palette) {
          vec clr =
              palette->color(round(p->rand * (palette->size() - 1))) % color;
          gfx::color(clr);
        }
        // img->draw(p->pos[0]-size*0.5, p->pos[1]-size*0.5, size, size);
        gfx::vertex(p->pos);
      }
      gfx::endVertices();
      gfx::enablePointSprites(false, 0);
      img->unbind();
    } else {
      for (int id : active) {
        Particle* p = &pool[id];
        gfx::fillCircle(p->pos, size);
      }
    }
  }
};
//
//
// def centripetal_paramterization(X, alpha):
//    u = geom.chord_lengths(X[:2,:])
//    u = u**alpha
//    u = np.cumsum(np.concatenate([[0.0],u]))
//    u = u/u[-1]
//    f = interp1d(u, X)
//    return f
//
// def random_radial_force(amt):
//    th = np.random.uniform(0, np.pi*2)
//    f = vec2(np.cos(th), np.sin(th))*np.random.normal()*amt
//    return f
//
// def centripetal_force(th, amt):
//    f = vec2(np.cos(th), np.sin(th))*amt
//    return f
//

class PathAttractor : public Force {
 public:
  mat    P;
  double kp                  = 10;
  double damping_ratio       = 0.1;
  double rotational_force    = 1000.;
  double random_radial_force = 0;
  double rand_start          = 0.;
  double rot_speed           = 4.;
  double centripetal_alpha   = 0.;
  mat    rand;
  vec    u;

  PathAttractor() {
    P    = zeros(2, 0);  //(2,0))
    rand = random::uniform(0, 1, 1300);
  }

  void update(double dt) {
    if (P.n_cols < 2) return;
    u = centripetal_parameterization(P, centripetal_alpha);
  }

  vec get(Particle* p) {
    if (P.n_cols < 2) return zeros(2);
    rowvec r = rand.rows(0, P.n_cols - 1).t();
    mat    X = join_vert(P, r);
    double t = (p->t / p->lifetime) * 1.02;
    t        = std::min(t, 1.);
    //
    vec x = ag::interpolate(X, u, ones(1) * t);
    // vec x = interpolate(X, t);
    double v = 1.;  // x(2);
    x        = x.subvec(0, 1);
    // gfx::color(1,0,0);
    // gfx::fillCircle(x, 10);
    double damp_ratio = (0.5 + p->rand * 0.5) * damping_ratio;
    double kp         = this->kp * 0.5 + v * this->kp * 0.5;

    double kv = damp_ratio * 2. * sqrt(kp);
    vec    f  = -kv * p->vel + kp * (x - p->pos);

    double th =
        rot_speed * p->rand2 * t * p->lifetime * ag::pi + p->rand * ag::pi * 2;
    f += vec({cos(th), sin(th)}) * p->rand * rotational_force * 10;

    double thr = ag::random::uniform(0, ag::pi * 2);
    f += vec({cos(thr), sin(thr)}) * ag::random::normal() * random_radial_force * 20;
    // std::cout << random_radial_force << std::endl;
    // def random_radial_force(amt):
    //    th = np.random.uniform(0, np.pi*2)
    //    f = vec2(np.cos(th), np.sin(th))*np.random.normal()*amt
    //    return f
    //
    // f += vec()
    return f;
  }
};

struct ParticlePath {
  ParticlePath(int pool_size) : particles(pool_size) {
    // particles.forces.push_back(&attract);
  }

  PathAttractor attract;
  Particles     particles;
  Interval      interval;
};

class FontStylizationParticle : public FontStylizationBase {
 public:
  struct {
    double time            = 0.;
    bool   active          = false;
    float  stroke_duration = 0.25;
  } screen_recording;

  struct {
    float kp                  = 30;
    float damping_ratio       = 0.5;
    float num_iterations      = 1;
    float num_particles       = 1000;
    float steps_per_second    = 100.;
    float spawn_interval      = 1;
    float rotational_force    = 1000;
    float random_radial_force = 0;
    float rand_start          = 0;
    float rot_speed           = 4.;
    float fade                = 0.0;
    float psize               = 10;
    float centripetal_alpha   = 0.;
    V4    color               = {0., 0., 0., 1.};
    bool  reset               = true;
    float speed               = 100.;
    bool  also_opposite       = true;
  } particle_params;

  Image         particle_img;
  RenderTarget* rt = 0;

  std::vector<ParticlePath> particle_paths;

  Image soft_brush_img;

  PolylineList stroke_outlines;
  PolylineList handwritten_strokes;

  double noise01(const vec& x) {
    return noise(x);  //*0.5 + 0.5;
  }

  FontStylizationParticle()
      : FontStylizationBase("Particle Font Stylization",
                            "./data/presets/stylization_particle") {
    render_data.softie_params.smoothness = 0;

    ParamList* child;

    // gui_params.newChild("Particles");
    param_modified << gui_params.addFloat("num_particles",
                                          &particle_params.num_particles,
                                          100,
                                          2000);
    gui_params.addBool("also opposite", &particle_params.also_opposite)
        ->describe("Spawn particles in both directions along path");
    gui_params.addFloat("kp", &particle_params.kp, 0, 1300);
    gui_params.addFloat("damping_ratio", &particle_params.damping_ratio, 0, 6);
    gui_params.addFloat("num_iterations",
                        &particle_params.num_iterations,
                        1,
                        100);
    gui_params.addFloat("steps_per_second",
                        &particle_params.steps_per_second,
                        100,
                        10000)
        ->describe("1/dt");
    gui_params.addFloat("spawn_interval",
                        &particle_params.spawn_interval,
                        1,
                        30);
    gui_params.addFloat("rotational_force",
                        &particle_params.rotational_force,
                        0,
                        10000)
        ->describe("Rotating force along path");
    gui_params.addFloat("random_radial_force",
                        &particle_params.random_radial_force,
                        0,
                        10000)
        ->describe("Force with a random direction at each step");
    gui_params.addFloat("rand_start", &particle_params.rand_start, 0, 150);
    gui_params.addFloat("rot_speed", &particle_params.rot_speed, -40, 40);
    gui_params.addFloat("fade", &particle_params.fade, 0, 0.1);
    gui_params.addFloat("psize", &particle_params.psize, 0.1, 50);
    gui_params.addColor("p color", &particle_params.color);
    gui_params.addFloat("centripetal_alpha",
                        &particle_params.centripetal_alpha,
                        0,
                        1.);
    param_modified << gui_params.addFloat("speed",
                                          &particle_params.speed,
                                          10,
                                          1300.);

    gui_params.newChild("Stroke");
    gui_params.addFloat("w0", &render_data.style.w0, 0., 2.)
        ->describe("Min width ratio (with respect to max stroke width)");
    gui_params.addFloat("w1", &render_data.style.w1, 0., 2.)
        ->describe("Max width ratio (with respect to max stroke width)");
    gui_params.addFloat("thickness", &render_data.width_mul, 0.1, 14.)
        ->describe("global thickness variation");

    // Finalize
    load_data();

    soft_brush_img = Image("data/images/p.png");

    // Because we are using render targets here, don't set view transform
    // in FontStylizationBase
    view_transform = false;
    shift_original = false;

    params.frame_seed = false;
  }

  void mode_init() {
    particle_img = Image(appWidth(), appHeight(), Image::BGRA);
  }

  void mode_exit() {
    if (rt) delete rt;
  }

  void mode_gui() {
    if (Keyboard::pressed(ImGuiKey_Enter) || param_modified.modified()) {
      particle_params.reset = true;
    }

    ImGui::SliderFloat("Stroke duration",
                       &screen_recording.stroke_duration,
                       0.1,
                       2.);
    if (ImGui::Button("reset time")) {
      screen_recording.time = 0;
    }
  }

  void draw_soft_brush(Image& img, const vec& pos, float radius) {
    img.draw(pos[0] - radius * 0.5, pos[1] - radius * 0.5, radius, radius);
  }

  void* getData() { return &gui_params; }

  void update_particle_image() {
    int w = appWidth();
    int h = appHeight();
    //        if (particle_img.width() != w || particle_img.height() != h ||
    //        particle_params.reset)
    //        {
    //            particle_img = Image(w, h);//, Image::BGRA);
    //            gfx::clear(background_params.color);
    //            particle_img.grabFrameBuffer();
    //        }

    if (!rt || rt->getWidth() != w || rt->getHeight() != h ||
        particle_params.reset) {
      if (rt) delete rt;
      rt = new RenderTarget(
          w,
          h,
          Texture::A32B32G32R32F);  // A8R8G8B8);//, Image::BGRA);
      rt->bind(false);
      gfx::clear(background_params.color);
      rt->unbind(false);
      // particle_img.grabFrameBuffer();
    }
  }

  void update_particles() {
    if (handwritten_strokes.size() != particle_paths.size() ||
        particle_params.reset || param_modified.modified()) {
      particle_paths.clear();
      for (int i = 0; i < handwritten_strokes.size(); i++) {
        // double l = ag::chord_length(handwritten_strokes[i])/10;
        // int n = particle_params.num_particles*pow(l,
        // particle_params.length_power);//ag::chord_length(handwritten_strokes[i]);//.size();
        ParticlePath pp(5000);
        particle_paths.push_back(pp);
        // particle_paths.back().particles.forces.push_back(&particle_paths.back().attract);
      }
    }

    for (int i = 0; i < particle_paths.size(); i++) {
      ParticlePath& pp               = particle_paths[i];
      pp.attract.P                   = handwritten_strokes[i].mat();
      pp.attract.kp                  = particle_params.kp;
      pp.attract.damping_ratio       = particle_params.damping_ratio;
      pp.attract.kp                  = particle_params.kp;
      pp.attract.rotational_force    = particle_params.rotational_force;
      pp.attract.random_radial_force = particle_params.random_radial_force;
      pp.attract.rand_start          = particle_params.rand_start;
      pp.attract.rot_speed =
          particle_params
              .rot_speed;  // ag::random::uniform(-particle_params.rot_speed,
                           // particle_params.rot_speed);
      pp.interval.tempo            = particle_params.spawn_interval;
      pp.attract.centripetal_alpha = particle_params.centripetal_alpha;
      if (pp.interval.tick(1)) {
        pp.particles.spawn(
            handwritten_strokes[i][0] +
                random::normal(2) * particle_params.rand_start,
            ag::chord_length(handwritten_strokes[i]) / particle_params.speed);
      }
    }

    int    n_iter = particle_params.num_iterations;
    double dt     = 1. / particle_params.steps_per_second;
    for (int it = 0; it < n_iter; it++) {
      for (int i = 0; i < particle_paths.size(); i++) {
        ParticlePath& pp = particle_paths[i];
        pp.particles.update(dt, &pp.attract);
        pp.particles.render(particle_params.psize,
                            particle_params.color,
                            &soft_brush_img,
                            &palette.palette);
      }
    }
  }

  void mode_render(bool needs_update) {
    palette.update();

    if (Keyboard::pressed(ImGuiKey_Enter)) {
      screen_recording.time = 0;
      random::seed(params.seed);
    }

    if (screen_recording.active) {
      screen_recording.time += 1. / (30 * screen_recording.stroke_duration);
    } else {
      // screen_recording.time += (1./100)*phunk_params.anim_speed;
      screen_recording.time += (1. / 100);  //*phunk_params.anim_speed;
    }

    if (screen_recording.active &&
        screen_recording.time > handwritten_strokes.size() + 2) {
      gfx::endScreenRecording();
      screen_recording.active = false;
    }

    double time = screen_recording.time;

    update_particle_image();

    // random::seed(params.seed);
    palette.update_lowpass(0.08);

    gfx::setBlendMode(gfx::BLENDMODE_ALPHA);

    // gfx::clear(background_params.color);
    rt->bind();
    // gfx::clear(background_params.color);
    gfx::setOrtho(appWidth(), appHeight());
    gfx::color(1);
    // particle_img.draw(0,0);
    gfx::color(
        ag::Color::with_alpha(background_params.color, particle_params.fade));
    gfx::fillRect(0, 0, appWidth(), appHeight());
    // gfx::color(1,0,0);
    // gfx::fillRect(0, 0, 10, 256);

    if (false)  // true) //!save_png)
    {
      palette.draw({0, appHeight() - 30}, 30);
    }

    // PolylineList     shape                = instance_string_shape(str);
    // PolylineList     skeletons            = instance_string_skeletons(str);
    StrokeRenderData render_data_for_mode = render_data;
    handwritten_strokes.clear();

    for (int i = 0; i < str.size(); i++) {
      // PolylineList strokes = smooth_glyph(str[i], render_data);
      std::vector<mat> strokes = sweep_glyph(str[i], 5., render_data, 1.);
      for (int j = 0; j < strokes.size(); j++) {
        handwritten_strokes.push_back(
            affine_mul(view * centerm, Polyline(strokes[j].rows(0, 1), false)));
        if (particle_params.also_opposite) {
          handwritten_strokes.push_back(
              affine_mul(view * centerm,
                         Polyline(fliplr(strokes[j].rows(0, 1)), false)));
        }
      }
    }

    // handwritten_strokes = affine_mul(view*centerm, smooth_string(str,
    // render_data_for_mode));
    // shape = affine_mul(view * centerm, shape);

    update_particles();

    // particle_img.grabFrameBuffer();
    rt->unbind();

    //
    gfx::clear(0, 0, 0, 1);
    gfx::setOrtho(appWidth(), appHeight());
    gfx::color(1);
    rt->getTexture()->bind();
    gfx::drawUVQuad(0, 0, appWidth(), appHeight());
    rt->getTexture()->unbind();

    /* vec clr = ag::Color::complementary(background_params.color, true); */
    /* gfx::color(clr); */
    /* if (params.draw_original) { */
    /*   gfx::pushMatrix(); */
    /*   // gfx::translate(0, params.font_size*1.3); */
    /*   gfx::lineWidth(1.); */
    /*   gfx::draw(shape); */
    /*   gfx::popMatrix(); */
    /* } */

    if (params.draw_stroke_polygon) {
      gfx::color(0);
      //std::cout << "Drawing stroke poly\n";
      for (auto stroke : handwritten_strokes) gfx::draw(stroke);
    }

    particle_params.reset = false;
  }
};
