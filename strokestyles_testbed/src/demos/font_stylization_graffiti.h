#pragma once
#include "font_stylization_base.h"

class FontStylizationGraffiti : public FontStylizationBase {
 public:
  struct {
    bool arrows_active  = false;
    bool render_on_load = false;
  } mode_params;

  struct {
    float seed                  = 12;
    float fold_thresh           = 0.;
    float normal_thresh         = 1.;
    bool  disambiguate_overlaps = true;
    bool  draw_hidden           = false;
    bool  shuffle_z             = true;
    bool  layering_active       = true;
    bool  all_unions            = false;
    float min_outline_size      = 0;
    float scale_factor          = 1;
  } tracing_params;

  struct {
    bool active = false;

    float w           = 4;
    float w0          = 1;
    float w1          = 1;
    float miter_limit = 1.;
    float offset      = 0.;

    float thickness = 2.;
    V4    color     = V4(0, 0, 0, 1);

    float angle = 20.;

    float highlight_thickness = 2.;
    float highlight_alpha     = 1.;
    float shine_size          = 0.2;

    float num_highlights = 2;
    float seed           = 20;
  } outline_params;

  struct {
    float seed             = 5.;
    bool  fill_planar      = true;
    float rand_face_offset = 0.;
    float shape_offset_amt = 0;
    V4    color            = V4(1, 1, 1, 0.9);

    float effect_alpha = 0.5;

    float num_bubbles   = 300;
    float bubble_alpha  = 0.4;
    float bubble_radius = 500;

    float num_fill_part_verts = 20;
    int   num_fill_parts      = 10;

    float fill_area_thresh = 5;
  } fill_params;

  struct {
    bool  active       = true;
    float depth        = 15;
    float angle_thresh = 30;
    float luminosity   = 1.;
    float saturation   = 0.;
    float thicken      = 1.;
    float smooth_sigma = 1.;
    V4    color        = V4(0.0, 0.0, 0.0, 1.);
  } extrusion_params;

  struct {
    float seed              = 5.;
    float hue_offset        = 0.;
    float num_bubbles       = 100;
    float bubble_alpha      = 0.2;
    bool  auto_bubble_color = true;
    float bubble_variance   = 1.;
    V4    bubble_color      = V4(1., 0., 0., 1.);
    V4    color             = V4(0.9, 0.9, 0.9, 1.);

    int wall_cycle = -1;

    bool  drips_active      = false;
    float drip_variance     = 0.3;
    float num_bg_vertices   = 20;
    float fuzzyness         = 0.5;
    float drip_length       = 30;
    float drip_angle_offset = 0.;
    int   drip_num_offsets  = 3;
    float drip_offset_amt   = 100;
    float bg_shape_alpha    = 1.;
  } background_params;

  PaletteManager            palette;
  cm::ParamModifiedTracker  outline_modified;
  std::vector<PolylineList> outline;
  PolylineList              passpartout;
  Layering                  layering;

  Image              soft_brush_img;
  std::vector<Image> bubble_brush_imgs;
  Image              highlight_brush_img;
  std::vector<Image> wall_imgs;

  std::vector<StylizedGlyph> glyphs;

  ArrowHeadParams arrow_params;

  std::vector<segment>  highlighted_segments;
  lines::Buffer         outline_buf;
  lines::Buffer         highlight_buf;
  Mesh                  whole_mesh;
  Mesh                  whole_buffered_mesh;
  PolylineList          extrusion;
  std::vector<Polyline> fill_faces;
  std::vector<Mesh>     fill_meshes;
  std::vector<vec>      face_offsets;

  cm::ParamModifiedTracker outl_param_modified;

  bool must_render = false;

  FontStylizationGraffiti(const std::string& name = "Graffiti Font Stylization")
      : FontStylizationBase(name, "./data/presets/stylization_graffiti"),
        palette("./data/palettes") {
    ParamList*               child;
    std::vector<std::string> spine_types = {"constant",
                                            "lerp",
                                            "sin",
                                            "chisel",
                                            "stepwise"};
    gui_params.addBool("render on load", &mode_params.render_on_load);
    param_modified
        << gui_params.addSelection("spine type",
                                   spine_types,
                                   &render_data.style.spine_type)
               ->describe("determines the kind of width variation used in the spine");

    param_modified
        << gui_params.addFloat("w0", &render_data.style.w0, 0., 2.)
               ->describe("Min width ratio (with respect to max stroke width)");
    param_modified
        << gui_params.addFloat("w1", &render_data.style.w1, 0., 2.)
               ->describe("Max width ratio (with respect to max stroke width)");
    param_modified << gui_params.addFloat("Chisel angle",
                                          &render_data.style.chisel_ang,
                                          -180,
                                          180)
                          ->describe("Orientation of chiseled tip");
    param_modified
        << gui_params.addFloat("min w", &render_data.style.min_width, 0., 100.)
               ->describe("Minimum spine width (global)");
    // param_modified << gui_params.addFloat("min thickness",
    // &render_data.min_width, 0., 100.)->describe("minimum thickness (in
    // original letter units)");//->noGui();
    param_modified
        << gui_params.addFloat("thickness", &render_data.width_mul, 0.1, 4.)
               ->describe("global thickness variation");

    child = gui_params.newChild("Arrows");
    param_modified << gui_params.addBool("Arrows actice",
                                         &mode_params.arrows_active);
    param_modified << gui_params.addFloat("head_w",
                                          &arrow_params.head_w,
                                          0.1,
                                          3.);
    param_modified << gui_params.addFloat("head_l",
                                          &arrow_params.head_l,
                                          1.,
                                          3.);
    param_modified << gui_params.addFloat("ang_0",
                                          &arrow_params.ang_0,
                                          90,
                                          160.);
    param_modified << gui_params.addFloat("ang_1", &arrow_params.ang_1, 0, 45.);
    param_modified << gui_params.addFloat("edge_l",
                                          &arrow_params.edge_l,
                                          0.,
                                          1);
    param_modified << gui_params.addFloat("tip_w",
                                          &arrow_params.tip_w,
                                          0.,
                                          0.3);
    param_modified << gui_params.addFloat("side_ang",
                                          &arrow_params.side_ang,
                                          0.,
                                          50);

    child = gui_params.newChild("Stylization");
    param_modified << gui_params.addBool("rectify", &render_data.rectify);

    param_modified
        << gui_params
               .addFloat("isotropy",
                         &render_data.softie_params.isotropy,
                         0.1,
                         1.)
               ->describe(
                   "isotropy of movement primitives (precision ellipses)");
    param_modified << gui_params
                          .addFloat("precision angle",
                                    &render_data.softie_params.precision_angle,
                                    -180,
                                    180)
                          ->describe(
                              "orientation of movement precision ellipse");
    param_modified << gui_params
                          .addFloat("global smoothness",
                                    &render_data.softie_params.smoothness,
                                    0.001,
                                    50)
                          ->describe("global smoothness of letters");
    param_modified << gui_params.addBool("opt x0",
                                         &render_data.softie_params.opt_x0);
    //        param_modified << gui_params.addFloat("width smoothing",
    //        &render_data.width_smoothing, 0.001, 1000)->describe("global
    //        smoothness of width");
    // param_modified << gui_params.addFloat("smoothing amt",
    // &render_data.style.smoothness, 0.0, 1.)->describe("influence of global
    // smoothing on letter");
    param_modified << gui_params
                          .addFloat(
                              "activation sigma",
                              &render_data.softie_params.lqt.activation_sigma,
                              0.0,
                              1.)
                          ->describe("Activation time sigma (spread)");
    param_modified
        << gui_params
               .addFloat("raidus scale", &render_data.radius_scale, 10.0, 400.)
               ->describe("Curvature radius scale for smoothing estimation");
    param_modified << gui_params
                          .addFloat("corner cov",
                                    &render_data.corner_cov_multiplier,
                                    0.,
                                    1)
                          ->describe("Smoothing for corners");
    param_modified << gui_params
                          .addFloat("corner thresh",
                                    &render_data.corner_smoothness_thresh,
                                    0.,
                                    1.0)
                          ->describe("Smoothness threshold for corners");
    param_modified << gui_params
                          .addFloat("smooth power",
                                    &render_data.smooth_power,
                                    0.,
                                    2.0)
                          ->describe("Power for radius to smoothness comp");
    param_modified << gui_params.addFloat("miter limit",
                                          &skel_params.miter_limit,
                                          0.09,
                                          4.);
    // param_modified << gui_params.addFloat("free end extension",
    // &render_data.free_end_extension, 0., 4.0)->describe("Extension for
    // terminals"); param_modified << gui_params.addFloat("extension x",
    // &render_data.extension_x, 0.0, 1.0)->describe("x axis extension amount");
    // param_modified << gui_params.addFloat("extension y",
    // &render_data.extension_y, 0.0, 1.0)->describe("y axis extension amount");
    param_modified << gui_params.addBool("unfold", &skel_params.unfold);
    // gui_params.addBool("stepwise", &params.stepwise)->describe("if toggled,
    // use random widths between w0 and w1 for each segment\notherwise use fixed
    // width");
    param_modified << gui_params.addFloat("fold angle sigma",
                                          &skel_params.fold_angle_sigma,
                                          0.01,
                                          ag::pi);

    param_modified
        << gui_params
               .addFloat("flat angle smoothing",
                         &render_data.softie_params.flat_angle_smooth_amt,
                         0.0,
                         50.)
               ->describe("smoothing relative to turning angle of polyline");
    param_modified << gui_params
                          .addFloat(
                              "flat angle multiplier",
                              &render_data.softie_params.flat_angle_multiplier,
                              0.001,
                              3.)
                          ->describe(
                              "angle multiplier for turning angle smoothing ");

    param_modified << gui_params
                          .addBool("force periodic",
                                   &render_data.force_periodic)
                          ->describe("enforce periodic smoothing");
    param_modified << gui_params
                          .addFloat("cap smoothness",
                                    &render_data.softie_params.cap_smoothness,
                                    0.1,
                                    2.)
                          ->describe(
                              "determines the smoothness of caps for periodic "
                              "smoothie (lower -> sharper)");

    gui_params.newChild(child, "Advanced")->appendOption("hidden");
    param_modified << gui_params.addInt("order",
                                        &render_data.softie_params.lqt.order);
    param_modified << gui_params.addInt("subdivision",
                                        &render_data.softie_params.subdivision);
    param_modified << gui_params.addInt("subsampling",
                                        &render_data.softie_params.subsampling);
    // param_modified << gui_params.addInt("ref subdivision",
    //                                    &render_data.ref_subdivision);
    param_modified << gui_params
                          .addFloat("ref spine width",
                                    &render_data.ref_spine_width,
                                    0.1,
                                    20.)
                          ->describe(
                              "Spine width for rectification trajectories");
    param_modified << gui_params.addFloat("rectify sleeve eps",
                                          &render_data.rectify_sleeve_eps,
                                          0.,
                                          100);
    param_modified << gui_params
                          .addInt("period",
                                  &render_data.softie_params.lqt.period)
                          ->describe("Periodicity overlap");

    param_modified << gui_params.addFloat("spine offset amt",
                                          &render_data.offset_amt,
                                          0.0,
                                          1.5);

    param_modified << gui_params
                          .addFloat("end weight",
                                    &render_data.softie_params.lqt.end_weight,
                                    0.0,
                                    500.)
                          ->describe(
                              "weight to enforce 0 end condition (disable?)");
    param_modified << gui_params
                          .addFloat(
                              "end cov scale",
                              &render_data.softie_params.lqt.end_cov_scale,
                              0.01,
                              1.)
                          ->describe(
                              "scaling for end covariance of non periodic "
                              "trajectories (not necesary?)");

    param_modified
        << gui_params
               .addFloat("force interpolation",
                         &render_data.softie_params.lqt.force_interpolation,
                         0.0,
                         100.)
               ->describe("Forces trajectory to go through key-points");
    param_modified << gui_params
                          .addFloat(
                              "centripetal alpha",
                              &render_data.softie_params.lqt.centripetal_alpha,
                              0.0,
                              1.)
                          ->describe(
                              "Centripetal parameter (0. - uniform, 0.5 - "
                              "centripetal, 1. - chordal)");
    param_modified << gui_params
                          .addFloat(
                              "max displacement",
                              &render_data.softie_params.lqt.max_displacement,
                              0.0,
                              10.)
                          ->describe("Max displacement for smoothing");

    param_modified << gui_params.addInt("polygon split",
                                        &render_data.softie_params.n_split);

    // gui_params.addFloat("stroke dur", &softie_params.stroke_duration, 0.01,
    // 0.4);

    child = gui_params.newChild("Layering");  //->appendOption("hidden");
    param_modified << gui_params.addBool("layering active",
                                         &tracing_params.layering_active);
    param_modified << gui_params.addBool("crossing unions",
                                         &render_data.crossing_unions);
    // gui_params.addFloat("min outline area", &tracing_params.min_outline_area,
    // 0., 20);
    param_modified << gui_params.addBool("all unions",
                                         &tracing_params.all_unions);
    gui_params.addBool("draw hidden", &tracing_params.draw_hidden);
    param_modified << gui_params
                          .addFloat("z seed", &tracing_params.seed, 1, 1000)
                          ->describe("layering depth random seed");
    param_modified << gui_params
                          .addBool("shuffle z", &tracing_params.shuffle_z)
                          ->describe("if true random order of z is used");
    param_modified << gui_params.addBool("disambiguate overlaps",
                                         &tracing_params.disambiguate_overlaps);
    param_modified
        << gui_params
               .addFloat("fold thresh", &tracing_params.fold_thresh, 0., 1.)
               ->describe("threshold for limiting fold lengths, 0 -> no fold");
    param_modified << gui_params
                          .addFloat("scale factor",
                                    &tracing_params.scale_factor,
                                    0.1,
                                    5.)
                          ->describe("scale factor for planar map");

    gui_params.addInt("max permutations", &Layering::max_permutations);
    gui_params.addFloat("min outline size",
                        &tracing_params.min_outline_size,
                        0.,
                        50);

    gui_params.newChild(child, "Advanced")->appendOption("hidden");
    param_modified << gui_params.addFloat("normal thresh",
                                          &tracing_params.normal_thresh,
                                          0.,
                                          10.);

    gui_params.newChild("Fill");
    outline_modified << gui_params.addFloat("fill seed", &fill_params.seed, 1, 1000);
    outline_modified << gui_params.addBool("geometric effects", &fill_params.fill_planar);
    outline_modified << gui_params.addFloat("random offset",
                                            &fill_params.rand_face_offset,
                                            0.,
                                            500);
    outline_modified << gui_params.addFloat("shape offset amount",
                                            &fill_params.shape_offset_amt,
                                            0.,
                                            100);

    gui_params.addFloat("face alpha", &fill_params.effect_alpha, 0.0, 1.);
    gui_params.addColor("fill color", &fill_params.color);
    outline_modified << gui_params.addFloat("sqrt fill area thresh",
                                            &fill_params.fill_area_thresh,
                                            0.0,
                                            500.);

    gui_params.addFloat("num bubbles", &fill_params.num_bubbles, 0.0, 100);
    gui_params.addFloat("bubble alpha", &fill_params.bubble_alpha, 0.0, 1);
    gui_params.addFloat("bubble radius", &fill_params.bubble_radius, 1., 700.);

    gui_params.addSelection("palette", palette.names, &palette.selected);

    gui_params.newChild("Outline and Shadow");
    outline_modified << gui_params.addFloat("line thickness", &outline_params.thickness, 0.5, 8.);

    outline_modified << gui_params.addFloat("out angle",
                                            &outline_params.angle,
                                            -180,
                                            180);
    outline_modified << gui_params.addFloat("outl w",
                                            &outline_params.w,
                                            4,
                                            70.);
    outline_modified << gui_params.addFloat("outl w0",
                                            &outline_params.w0,
                                            0.05,
                                            1);
    outline_modified << gui_params.addFloat("outl w1",
                                            &outline_params.w1,
                                            0.05,
                                            1);
    outline_modified << gui_params.addFloat("outl miter limit",
                                            &outline_params.miter_limit,
                                            0.09,
                                            4.);
    gui_params.addColor("outl color", &outline_params.color);
    gui_params.addFloat("outl offset", &outline_params.offset, -2, 2);

    gui_params.newChild("Highlights");
    outline_modified << gui_params.addFloat("highlight thickness",
                                            &outline_params.highlight_thickness,
                                            0.5,
                                            8.);
    outline_modified << gui_params.addFloat("light dir", &outline_params.angle, -180, 180);
    gui_params.addFloat("highlight alpha",
                        &outline_params.highlight_alpha,
                        0.0,
                        1.);
    gui_params.addFloat("shine size", &outline_params.shine_size, 0.0, 100.);
    gui_params.addFloat("num highlights", &outline_params.num_highlights, 0, 5)
        ->describe("number of highlight starts");

    gui_params.newChild("Extrusion");
    outline_modified << gui_params.addBool("extrusion active", &extrusion_params.active);
    outline_modified << gui_params.addFloat("extruion depth", &extrusion_params.depth, 1, 400.);
    outline_modified << gui_params.addFloat("extr angle thresh",
                                            &extrusion_params.angle_thresh,
                                            1,
                                            90.);
    outline_modified << gui_params.addFloat("smooth sigma", &extrusion_params.smooth_sigma, 0, 10.);

    outline_modified << gui_params.addFloat("thicken", &extrusion_params.thicken, 0., 20.);
    outline_modified << gui_params.addFloat("luminosity", &extrusion_params.luminosity, 0, 2.);
    outline_modified << gui_params.addFloat("saturation", &extrusion_params.saturation, 0, 2.);
    outline_modified << gui_params.addColor("extrusion color", &extrusion_params.color);

    gui_params.newChild(child, "Background shape");

    gui_params.addFloat("background seed", &background_params.seed, 1, 300);
    gui_params.addBool("drips active", &background_params.drips_active)
        ->describe(
            "activate drippy background (note, to deactivate bubbles just set "
            "num bubbles to 0)");
    gui_params.addBool("auto bubble color",
                       &background_params.auto_bubble_color);
    gui_params.addColor("bubble color", &background_params.bubble_color);
    gui_params.addFloat("bubble hue offset",
                        &background_params.hue_offset,
                        0,
                        360);
    gui_params
        .addFloat("drip variance", &background_params.drip_variance, 0.01, 1.)
        ->describe("extent of drippy background");
    gui_params
        .addFloat("num verices", &background_params.num_bg_vertices, 6, 40)
        ->describe("num vertices of base drippy polygon");
    gui_params.addFloat("fuzzyness", &background_params.fuzzyness, 0, 1)
        ->describe("random offsets in drippy polygon");
    gui_params.addFloat("drip length", &background_params.drip_length, 10, 300)
        ->describe("length of drip with respect to polygon");
    gui_params
        .addFloat("drip angle offset",
                  &background_params.drip_angle_offset,
                  0,
                  180)
        ->describe(
            "angular parameter for drips, 0: round, 1:angular (more flame "
            "like)");
    gui_params.addInt("num offset steps", &background_params.drip_num_offsets)
        ->describe("Number of offsets applied to drip poly");
    gui_params
        .addFloat("shape offset amt",
                  &background_params.drip_offset_amt,
                  -200,
                  200)
        ->describe("global amount of offset");

    gui_params.addInt("wall image cycle", &background_params.wall_cycle)
        ->describe("wall image, -1 is not wall");

    soft_brush_img      = Image("data/images/blurry-01.png");
    highlight_brush_img = Image("data/images/highlight.png");

    std::vector<std::string> wall_files = getFilesInFolder("data/images/walls");
    for (const std::string& path : wall_files) wall_imgs.push_back(Image(path));

    std::vector<std::string> bubble_files =
        getFilesInFolder("data/images/background");
    for (const std::string& path : bubble_files)
      bubble_brush_imgs.push_back(Image(path));

    // Finalize
    load_data();
  }

  void activated() {
    if (mode_params.render_on_load)
      must_render = true;
  }

  void mode_gui() {
    if ((Keyboard::pressed(ImGuiKey_Enter)) && tracing_params.layering_active) {
      compute_layering();
    }

    ImGui::Text("Press Enter to Render");
    if (Keyboard::pressed(ImGuiKey_UpArrow))
      palette.selected = mod(palette.selected - 1, palette.size());
    if (Keyboard::pressed(ImGuiKey_DownArrow))
      palette.selected = mod(palette.selected + 1, palette.size());
  }

  void mode_render(bool needs_update) {
    gfx::enableAntiAliasing(true);
    // make sure subdivision is always > 1
    if (render_data.softie_params.subdivision <= 2)
      render_data.softie_params.subdivision = 2;

    if (needs_update)  // param_modified.modified() || params.rt_update)
    {
      // render_data.corner_cov_multiplier = 0.;
      ArrowHeadGenerator arrows(arrow_params);
      glyphs =
          glyph_string(str, skel_params, render_data, {&arrows}, {&arrows});

      layering                        = Layering();
      outline_modified.force_modified = true;
      //            if (editor.strokes.size())
      //                graff_glyphs.push_back(editor.generate_glyph(render_data_for_mode));
      if ((params.rt_update || must_render) && tracing_params.layering_active)
        compute_layering();
    }

    must_render = false;

    gfx::color(0);
    if (!layering.whole.size() || params.debug_draw) {
      for (int i = 0; i < glyphs.size(); i++) {
        for (int j = 0; j < glyphs[i].chunks.size(); j++) {
          if (params.draw_stroke_polygon) {
            gfx::color(0.5);
            gfx::draw(glyphs[i].chunks[j].merged_skel.shape);
          }

          if (params.show_serifs && serifs.size()) {
            auto it = serifs[i].serifs.find(j);
            if (it != serifs[i].serifs.end()) {
              gfx::color(1, 0, 0, 0.5);
              gfx::fill(glyphs[i].chunks[j].shape);
            }
          }

          // stroke outlines
          gfx::color(0.0);
          gfx::lineWidth(params.chunk_line_width);
          gfx::draw(glyphs[i].chunks[j].shape);
          gfx::lineWidth(1.);

          if (glyphs[i].chunks[j].has_start_cap())
            gfx::draw(glyphs[i].chunks[j].get_start_cap());
          if (glyphs[i].chunks[j].has_end_cap())
            gfx::draw(glyphs[i].chunks[j].get_end_cap());

          // spines
          gfx::lineWidth(params.chunk_line_width);
          //gfx::lineStipple(2.);
          gfx::color(1, 0, 0, 1);
          gfx::draw(glyphs[i].chunks[j].spine.points);
          //gfx::lineStipple(0);
          gfx::lineWidth(1.);

          //                    gfx::color(1,0,0);
          //                    for (int k = 0; k <
          //                    graff_glyphs[i].join_caps.size(); k++)
          //                    {
          //                        gfx::fill(graff_glyphs[i].join_caps[k].cap);
          //                    }
        }

        // graff_glyphs[i].compute_unions(tracing_params.union_thresh);
      }
    }

    if (!params.debug_draw && layering.whole.size()) {
      if (gfx::isRenderingToEps())
        do_fancy_stuff_save();
      else
        do_fancy_stuff();
    }
  }

  void compute_layering() {
    if (!glyphs.size()) return;

    layering = layer_glyphs(glyphs,
                            tracing_params.shuffle_z,
                            tracing_params.seed,
                            tracing_params.fold_thresh,
                            tracing_params.normal_thresh,
                            tracing_params.disambiguate_overlaps,
                            tracing_params.all_unions,
                            tracing_params.scale_factor,
                            std::vector<std::vector<Polyline>>());
  }

  void draw_soft_brush(Image& img, const vec& pos, float radius) {
    img.draw(pos[0] - radius * 0.5, pos[1] - radius * 0.5, radius, radius);
  }

  void do_fancy_stuff_save() {
    PolylineList whole_shape = shape_offset(
        shape_offset(layering.whole, tracing_params.min_outline_size / 2),
        -tracing_params.min_outline_size / 2);
    if (extrusion_params.thicken > 0.)
      whole_shape = shape_offset(whole_shape, extrusion_params.thicken, ag::JOIN_ROUND);

    gfx::color(outline_params.color);
    gfx::fill(whole_shape);  //was fill whole_shape);

    if (extrusion_params.active) {
      for (const Polyline& P : reversed(extrusion)) {
        vec clr = ag::Color::mul_saturation(extrusion_params.color,
                                            extrusion_params.saturation);
        clr     = ag::Color::mul_luminance(clr, extrusion_params.luminosity);

        gfx::color(clr);
        gfx::fill(P);
        gfx::color(outline_params.color);
        gfx::draw(P);
      }
    }

    vec clr = (vec)fill_params.color;
    gfx::color(clr);
    gfx::fill(layering.whole);

    // outline
    gfx::color(outline_params.color);
    set_line_width(outline_params.thickness);
    for (int i = 0; i < layering.visible_outlines.size(); i++) {
      gfx::draw(layering.visible_outlines[i]);
    }

    gfx::draw(layering.whole);
    //gfx::lineWidth(1.);
  }

  void do_fancy_stuff() {
    if (!layering.whole.size()) return;

    bool outl_dirty = false;
    if (outline_buf.empty() || highlight_buf.empty() || outline_modified.modified())
      outl_dirty = true;

    if (background_params.wall_cycle >= 0) {
      gfx::pushMatrix();
      gfx::identity();
      gfx::color(1);
      wall_imgs[mod(background_params.wall_cycle, wall_imgs.size())].draw(0, 0);
      gfx::popMatrix();
    }

    aa_box box = bounding_rect(layering.whole);
    // Set fillin seed
    PolylineList whole = shape_offset(
        shape_offset(layering.whole, tracing_params.min_outline_size / 2),
        -tracing_params.min_outline_size / 2);

    if (background_params.drips_active) {
      random::seed(background_params.seed);
      vec    cenp = rect_center(box);
      double w    = rect_width(box) * background_params.drip_variance * 3;
      double h    = rect_height(box) * background_params.drip_variance *
                 2;  // ag::distance(box[0], box[1])/2;
      Polyline bgpoly =
          random_elliptic_polygon(background_params.num_bg_vertices,
                                  w * (1. - background_params.fuzzyness),
                                  w,
                                  h * (1. - background_params.fuzzyness),
                                  h,
                                  cenp);

      vec bg_src_color = palette.random(background_params.bubble_alpha);
      vec bg_color     = background_params.bubble_color;

      vec offsets = flipud(linspace(0, 1, background_params.drip_num_offsets));
      for (double t : offsets) {
        PolylineList ofshape =
            shape_offset(bgpoly,
                         -background_params.drip_offset_amt +
                             t * (background_params.drip_offset_amt * 2),
                         ag::JOIN_MITER);
        PolylineList drippy =
            bubbly_shape(ofshape,
                         background_params.drip_length * (0.5 + t),
                         50,
                         ag::radians(background_params.drip_angle_offset));

        if (background_params.auto_bubble_color) {
          bg_src_color = palette.random(background_params.bubble_alpha);
          vec hsva     = ag::Color::rgb_to_hsv(bg_src_color);
          hsva(0)      = fmodf(hsva(0) + (background_params.hue_offset / 360), 1);
          if (hsva(1) > 0.5)
            hsva(1) *= 0.5;
          else
            hsva(1) *= 2.;
          bg_color = ag::Color::hsv_to_rgb(hsva);
        } else {
          bg_color = ag::Color::mul_luminance(background_params.bubble_color,
                                              0.3 + 0.7 * t);
        }

        gfx::color(bg_color);  // ag::Color::mul_luminance(bg_color, 0.3 +
                               // 0.7*(1.-t)));
        gfx::fill(drippy);
      }
    }

    // Set fillin seed
    random::seed(fill_params.seed);

    // light direction
    float theta = ag::radians(outline_params.angle);
    vec   ldir  = {cos(theta + pi / 2), sin(theta + pi / 2)};

    if (outl_dirty) {
      //Tessellator tess(whole);
      whole_mesh = gfx::tessellate(whole);  // tess.mesh;
    }

    if (extrusion_params.active) {
      if (outl_dirty) {
        PolylineList whole_shape = whole;  // layering.whole;
        if (extrusion_params.thicken > 0.)
          whole_shape =
              shape_offset(whole_shape, extrusion_params.thicken, ag::JOIN_ROUND);

        vec l = {cos(theta), sin(theta)};

        extrusion =
            slow_extrude(whole_shape,
                         ldir,
                         extrusion_params.depth,
                         ag::radians(extrusion_params.angle_thresh),
                         extrusion_params.smooth_sigma,
                         false);
        //extrusion = shape_union(extrusion, extrusion);
        //extrusion_mesh = gfx::tessellate(extrusion_shape);
        //Tessellator tess(whole_shape);
        whole_buffered_mesh = gfx::tessellate(whole_shape);  //tess.mesh;
      }
      // gfx::setBlendMode(gfx::BLENDMODE_NONE);
      for (const Polyline& P : reversed(extrusion)) {
        vec clr = ag::Color::mul_saturation(extrusion_params.color,
                                            extrusion_params.saturation);
        clr     = ag::Color::mul_luminance(clr, extrusion_params.luminosity);

        gfx::color(clr);
        gfx::fill(P);
        gfx::color(outline_params.color);

        gfx::draw(P);
      }

      gfx::color(outline_params.color);
      gfx::draw(whole_buffered_mesh);  //was fill whole_shape);
    }

    // Fill and create mask
    gfx::beginMask(true);
    gfx::color(1, 1, 1, 1);
    gfx::draw(whole_mesh);  // layering.whole);
    gfx::endMask();
    //
    // return;
    //        if (true) //params.debug_draw)
    //        {
    //            for (int i = 0; i < graff_glyphs.size(); i++)
    //            {
    //                for (int j = 0; j < graff_glyphs[i].strokes.size(); j++)
    //                {
    //                    gfx::lineStipple(2.);
    //                    gfx::color(0.0, 0.5);
    //                    gfx::draw(graff_glyphs[i].strokes[j].softie.shape);
    //                    gfx::lineStipple(0.);
    //                }
    //            }
    //        }

    // Set fillin seed
    random::seed(fill_params.seed);

    gfx::beginMasked();
    vec clr = palette.random() % (vec)fill_params.color;
    gfx::color(clr);
    gfx::draw(whole_mesh);  //whole);  // layering.whole);
    gfx::endMasked();

    // Bubbly effects
    int n_bubbles = fill_params.num_bubbles;
    if (n_bubbles) {
      // Set fillin seed
      random::seed(fill_params.seed);

      gfx::beginMasked();
      soft_brush_img.bind();
      float min_r = 0.4;
      for (int i = 0; i < n_bubbles; i++) {
        vec   p = random_point_in_rect(box);
        float r = fill_params.bubble_radius * min_r +
                  random::uniform() * fill_params.bubble_radius * (1 - min_r);
        gfx::color(palette.random(fill_params.bubble_alpha));
        draw_soft_brush(soft_brush_img, p, r);
      }
      soft_brush_img.unbind();
      gfx::endMasked();
    }

    int count = 0;
    // Geometric effects
    if (fill_params.fill_planar) {
      gfx::beginMasked();
      float fill_thresh =
          fill_params.fill_area_thresh * fill_params.fill_area_thresh;

      static PolylineList prev_faces;
      static ivec         prev_rand = ones<ivec>(1);

      if (prev_faces.size() != layering.interior_faces.size())
        prev_faces.clear();
      if (prev_rand.n_elem != layering.interior_faces.size())
        prev_rand = ones<ivec>(layering.interior_faces.size());

      // Set fillin seed
      ag::random::seed(fill_params.seed);

      int  c   = 0;
      ivec rnd = prev_rand;

      if (outl_dirty) {
        fill_faces.clear();
        fill_meshes.clear();
        face_offsets.clear();

        for (int i = 0; i < layering.interior_faces.size(); i++) {
          // for (auto r: rots)
          if (fabs(polygon_area(layering.interior_faces[i])) > fill_thresh)

          {
            double r = 0.;  // params.rotation;

            face_offsets.push_back((random::uniform(2) - 0.5) *
                                   fill_params.rand_face_offset);
            Polyline P = layering.interior_faces
                             [i];  // cleanup_polyline(layering.interior_faces[i],
                                   // true, 0.5);
            PolylineList S = {P};  // layering.interior_faces[i]};
            c++;
            float o = random::uniform() * fill_params.shape_offset_amt;
            if (o > 0) S = shape_offset(S, -o);
            fill_faces.push_back(layering.interior_faces[i]);
            fill_meshes.push_back(gfx::tessellate(S));

            count++;
          }
          auto v = random::randint();  // random::uniform();
          rnd(i) = v;
          // std::cout << v << ",";
        }
      }

      for (int i = 0; i < fill_faces.size(); i++) {
        //
        vec clr = palette.color(i, fill_params.effect_alpha);
        gfx::color(clr);  // random(0.3)); //random::uniform(), 0.5);
                          // //5);//gfx::defaultColor(i, 0.5));
        gfx::pushMatrix();
        //
        //                    gfx::translate(cenp);
        //                    gfx::rotate(r);//params.rotation);
        //                    gfx::translate(-cenp);
        gfx::translate(face_offsets[i]);

        gfx::draw(fill_meshes[i]);
        // diff(layering.interior_faces[i].mat(), 1, 1).print();
        //                    for (int j = 0; j < S[0].size(); j++)
        //                    {
        //                        printf("%.2f %.2f, ", S[0][j](0),
        //                        S[0][j](1));
        //                    }
        //                    printf("\n");
        // S[0].mat().print();
        // gfx::lineWidth(2.);
        gfx::translate((random::uniform(2) - 0.5) *
                       fill_params.rand_face_offset);
        set_line_width(0.25 + random::uniform() * 2);

        clr = palette.color(i * 3, fill_params.effect_alpha);
        gfx::color(clr);           // ag::Color::mul_luminance(clr, 0.5));
        gfx::draw(fill_faces[i]);  //layering.interior_faces[i], true);
        gfx::lineWidth(1.);
        //
        //
        gfx::popMatrix();
      }
      gfx::endMasked();
    }

    // std::cout << "Face count:" << count << std::endl;
    // return;

    // Layered highlights
    gfx::beginMasked();
    gfx::color(1, outline_params.highlight_alpha);
    //
    gfx::pushMatrix();
    gfx::translate(ldir(0) * outline_params.thickness,   //* 0.5,
                   ldir(1) * outline_params.thickness);  // * 0.5);

    if (global_debug.fancy_outlines) {
      if (outl_dirty) {
        highlight_buf.clear();
        random::seed(outline_params.seed);
        highlighted_segments.clear();
        for (int i = 0; i < layering.visible_segments.size(); i++) {
          vec n = layering.visible_segment_normals[i];
          if (!n.n_cols || !n.n_rows) continue;  // trouble here
          if (dot(n, ldir) > 0.) {
            //gfx::draw_segment(layering.visible_segments[i]);
            highlight_buf.segment(layering.visible_segments[i].a, layering.visible_segments[i].b, outline_params.highlight_thickness);
            highlighted_segments.push_back(layering.visible_segments[i]);
          }
        }
      }

      lines::draw(highlight_buf);
    } else {
      highlight_buf.clear();
      set_line_width(outline_params.highlight_thickness);
      random::seed(outline_params.seed);
      highlighted_segments.clear();
      for (int i = 0; i < layering.visible_segments.size(); i++) {
        vec n = layering.visible_segment_normals[i];
        if (!n.n_cols || !n.n_rows) continue;  // trouble here
        if (dot(n, ldir) > 0.) {
          gfx::draw_segment(layering.visible_segments[i]);
          //highlight_buf.segment(layering.visible_segments[i].a, layering.visible_segments[i].b, outline_params.highlight_thickness);
          highlighted_segments.push_back(layering.visible_segments[i]);
        }
      }
    }

    gfx::popMatrix();
    gfx::lineWidth(1.);
    gfx::endMasked();

    // Layered outline
    gfx::color(outline_params.color);
    //
    //lines::begin(outline_params.thickness);
    // for (const segment& seg: layering.visible_segments)
    // gfx::draw_segment(seg);
    if (global_debug.fancy_outlines) {
      if (outl_dirty) {
        outline_buf.clear();
        for (int i = 0; i < layering.visible_outlines.size(); i++) {
          aa_box box = bounding_rect(layering.visible_outlines[i]);
          double sw  = rect_width(box);
          double sh  = rect_height(box);
          // if (sw > tracing_params.min_outline_size || sh >
          // tracing_params.min_outline_size)
          //gfx::pushMatrix(trans_2d(ag::random::uniform(-10, 10, 2)));
          //gfx::draw(layering.visible_outlines[i]);
          //lines::draw(layering.visible_outlines[i]);
          outline_buf.polyline(layering.visible_outlines[i], outline_params.thickness);
          //gfx::popMatrix();
        }
      }

      lines::draw(outline_buf);
    } else {
      set_line_width(outline_params.thickness);
      outline_buf.clear();
      for (int i = 0; i < layering.visible_outlines.size(); i++) {
        aa_box box = bounding_rect(layering.visible_outlines[i]);
        double sw  = rect_width(box);
        double sh  = rect_height(box);
        // if (sw > tracing_params.min_outline_size || sh >
        // tracing_params.min_outline_size)
        //gfx::pushMatrix(trans_2d(ag::random::uniform(-10, 10, 2)));
        gfx::draw(layering.visible_outlines[i]);
        //lines::draw(layering.visible_outlines[i]);
        //outline_buf.polyline(layering.visible_outlines[i], outline_params.thickness);
        //gfx::popMatrix();
      }
    }
    //lines::end();

    // for (int i = 0; i < whole.size(); i++)
    // {
    //     aa_box box = bounding_rect(whole[i]);
    //     double a = rect_width(box) * rect_height(box);
    //     //if (a >
    //     tracing_params.min_outline_area*tracing_params.min_outline_area)
    //     gfx::draw(whole[i]);
    // }

    // gfx::draw(layering.whole);
    gfx::lineWidth(1.);

    if (tracing_params.draw_hidden) {
      gfx::lineStipple(4);
      gfx::color(0, 0.5);
      for (const segment& seg : layering.invisible_segments)
        gfx::draw_segment(seg);
      gfx::lineStipple(0);
    }

    // higjlights
    gfx::pushMatrix();
    gfx::translate(ldir(0) * outline_params.thickness,
                   ldir(1) * outline_params.thickness);
    gfx::color(1, outline_params.highlight_alpha);
    highlight_brush_img.bind();
    if (highlighted_segments.size())
      for (int i = 0; i < outline_params.num_highlights; i++) {
        segment seg = random::choice(highlighted_segments);
        gfx::pushMatrix();
        gfx::translate(seg[0] + ldir * outline_params.thickness * 0.5);
        gfx::rotate(outline_params.angle);
        float sz = outline_params.shine_size;
        draw_soft_brush(highlight_brush_img, zeros(2), sz);

        gfx::popMatrix();
      }
    highlight_brush_img.unbind();
    gfx::popMatrix();
  }
};
