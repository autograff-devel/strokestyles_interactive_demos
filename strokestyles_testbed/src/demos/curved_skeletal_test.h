#pragma once
#include "colormotor.h"
#include "common.h"
#include "autograff.h"
#include "tinysplinecpp.h"
#include "ag_skeletal_strokes.h"

using namespace cm;
using namespace ag;
using namespace arma;

class CurvedSkeletalTest : public AppModule
{
public:
    int tool=0;
    
    ContourMaker ctr_maker;
    
    ParamList gui_params;
    SoftieParams softie_params;
    
    struct
    {
        float w=70;
        float w0=1.;
        float w1=1.;
        float spine_theta=0.;
        int spine_type = 0;
        
        bool unfold=false;
        bool duplicate=false;
        bool adjust_index_map=true;

        float extrema_tol = 5;

        V4 fill = V4(1,1,1,0.9);
    }params;
    
    struct
    {
        float seed = 111;
        bool active = true;
        bool debug_folds = false;
        float fold_thresh = 0;
        float normal_thresh = 1.;
        bool disambiguate_overlaps = true;
        bool draw_hidden = true;
        bool shuffle_z = false;
        float outline_thickness = 1.;
    }layering_params;
    
    struct
    {
        int num_points=0;
        int num_control_points=0;
        mat res;
        mat res_spline;
    }stats;
    
    

    PolylineList svg_flesh;
    
    CurvedSkeletalTest()
    :
    AppModule("Curved Skeletal Test")
    {
        softie_params.lqt.order = 3; // make sure we have acceleration in the state

        std::vector<std::string> spine_types = {"stepwise", "constant", "lerp", "sin", "chisel"};
        
        gui_params.newChild("Skeletal");
        gui_params.addFloat("w", &params.w, 10, 200);
        gui_params.addFloat("w0", &params.w0, 0.01, 1.)->describe("minimum stroke width ratio"); //->appendOption("vslider");
        gui_params.addFloat("w1", &params.w1, 0.01, 1.)->describe("maximum stroke width ratio"); //->appendOption("vslider")->sameLine();
        gui_params.addSelection("spine type", spine_types, &params.spine_type)->describe("determines the kind of width variation used in the spine");
        gui_params.addFloat("chisel angle", &params.spine_theta, 0.0, PI*2)->describe("angle of chiseled spine"); // ->appendOption("knob")
        gui_params.addColor("fill color", &params.fill);
        gui_params.addBool("unfold", &params.unfold);
        gui_params.addBool("adjust index map", &params.adjust_index_map);
        gui_params.addFloat("extrema tol exp", &params.extrema_tol, 0.0, 20);
        
        ParamList* child = gui_params.newChild("Softie");
        gui_params.addFloat("smoothness", &softie_params.smoothness, 100, 600); //->appendOption("vslider");
        gui_params.addFloat("precision angle", &softie_params.precision_angle, 0., 360.)->describe("orientation of covariance ellipses"); //->sameLine();
        gui_params.addFloat("isotropy", &softie_params.isotropy, 0.1, 1.)->describe("determines the shape of the covariance ellipses, 1 -> spherical");
        
        gui_params.newChild(child, "LQT")->appendOption("hidden");
        gui_params.addInt("order", &softie_params.lqt.order)->describe("order of dynamical system, this determines the derivative that magnitude of which will be minimized");
        gui_params.addBool("augmented", &softie_params.lqt.augmented)->describe("internal, use augmented covariance to speed up computation");
        gui_params.addFloat("activation sigma", &softie_params.lqt.activation_sigma, 0.0, 1.)->describe("Activation time sigma (spread)");
        gui_params.addFloat("end weight", &softie_params.lqt.end_weight, 0.0, 100.)->describe("weight to enforce 0 end condition (disable?)");
        gui_params.addFloat("end cov scale", &softie_params.lqt.end_cov_scale, 0.001, 1.)->describe("scaling for end covariance of non periodic trajectories (not necesary?)");
        gui_params.addFloat("centripetal alpha", &softie_params.lqt.centripetal_alpha, 0.0, 1.)->describe("Centripetal parameter (0. - uniform, 0.5 - centripetal, 1. - chordal)");
        gui_params.addFloat("max displacement", &softie_params.lqt.max_displacement, 0.0, 1000.)->describe("Max displacement for smoothing");
        
        gui_params.newChild(child, "Timing")->appendOption("hidden");
        gui_params.addInt("subdivision", &softie_params.subdivision)->describe("subdivision of curves");
        
        child = gui_params.newChild("Layering");
        gui_params.addBool("layering enabled", &layering_params.active);
        gui_params.addBool("debug draw folds", &layering_params.debug_folds);
        gui_params.addBool("draw hidden", &layering_params.draw_hidden);
        gui_params.addFloat("z seed", &layering_params.seed, 1, 1000);
        gui_params.addBool("shuffle z", &layering_params.shuffle_z);
        gui_params.addBool("disambiguate overlaps", &layering_params.disambiguate_overlaps);
        gui_params.addFloat("fold thresh", &layering_params.fold_thresh, 0.001, 1.);
        gui_params.addFloat("outline thickness", &layering_params.outline_thickness, 1., 5.);
        gui_params.newChild(child, "Thresholds"); //->appendOption("hidden");
        gui_params.addFloat("normal thresh", &layering_params.normal_thresh, 0., 10.);


        gui_params.newChild("Debug");
        gui_params.addBool("debug draw", &softie_params.debug_draw);
        gui_params.addBool("draw gaussians", &softie_params.draw_gauss);
        gui_params.addBool("draw normals", &softie_params.draw_normals);
        gui_params.addBool("draw partitions", &softie_params.draw_partitions);
        
        // can access it with params.getFloat("foo")
        gui_params.loadXml(configuration_path(this, ".xml"));
        ctr_maker.load(configuration_path(this, ".csv")); 
        
        load_flesh();
    }
    
    void load_flesh()
    {
        svg_flesh = load_svg("data/svg/testflesh.svg");
        mat m = rect_in_rect_transform(bounding_rect(svg_flesh), make_rect(0,-0.5,20,1));
        svg_flesh = affine_mul(m, svg_flesh);
    }
    
    
    bool init()
    {
        OPENBLAS_THREAD_SET
        return true;
    }
    
    void exit()
    {
        gui_params.saveXml(configuration_path(this, ".xml"));
        ctr_maker.save(configuration_path(this, ".csv")); 
    }
    
    
    bool gui()
    {
        ui::begin();
        //ui::demo();
        tool = ui::toolbar("tools", "ab", tool);
        ctr_maker.interact(tool);
        ui::end();
        
        ImGui::Text("Num Control Points: %d", stats.num_control_points);
        ImGui::Text("Num Points: %d", stats.num_points);
        

        if( ImGui::Button("Clear") )
        {
            ctr_maker.clear();
        }
        
        imgui(gui_params); // Creates a UI for the parameters
        return false;
    }
    
    
    void update()
    {
        // Gets called every frame before render
    }

    vec curvature_precise(const mat& X)
    {
        if (X.n_rows < 6)
            return curvature(Polyline(X.rows(0,1), false));
        rowvec dx = X.row(2);
        rowvec dy = X.row(3);

        rowvec ddx = X.row(4);
        rowvec ddy = X.row(5);
    
        rowvec kappa = (dx%ddy - dy%ddx) / (pow(dx%dx + dy%dy, 3./2) + 1e-40);
        return kappa.t();
    }

    // Search for a local maximum bracket from the left
    static std::pair<int, int> max_bracket(const arma::vec& dy, double tol)
    {
        int n = dy.n_elem;
        int a, b;
        
        // HACK, if region is relatively flat force root
        double mx = fabs(max(dy));
        double mn = fabs(min(dy));
        if (mn <= tol || mx <= tol)
        {
            // given that our mapping is correct, it works well to use the midpoint
            int i = n / 2; //clamp(bidirectional_argmax(y), 0, n-1);
            return std::make_pair(i, i);
        }
        
        tol = tol * std::max(mx, mn);
        
        for (a=0; a < n; a++)
            if (dy[a] > tol)
                break;
        if (a>=n-1)
            return std::make_pair(n/2, n/2);
        for (b=a; b < n; b++)
            if (dy[b] < -tol)
                break;
        return std::make_pair(a, b);
    }
    

    int discrete_bisection(const arma::vec& y,
                            const std::pair<int, int>& bracket,
                            double tol,
                            int max_iter=100,
                            bool verbose=false)
    {
        int ny = y.n_elem;
        int a = bracket.first; int b = bracket.second;
#define f(x) y[std::max(0, std::min(x, ny-1))]
        if (f(a)*f(b) > 0)
            return -1;
        int c;
        
        for (int i = 0; i < max_iter; i++)
        {
            c = (a+b)/2;
            if (verbose)
            {
                std::cout << "a=" << a
                << ", b=" << b
                << ", c=" << c
                << ", err=" << (b-a)/2 << std::endl;
            }
            if (f(c)==0 || 0.5*(b-a) < 1)
                break;
            else
                if (f(c)*f(a) > 0)
                    a = c;
                else
                    b = c;
        }
#undef f
        return std::max(0, std::min(c, ny-1));
    }
    
    ivec adjust_index_map(const SketchResult& res, double tol)
    {
        ivec I = res.index_map;
        uvec subsub = linspace<uvec>(0, res.traj.size()-1, res.index_map.n_elem*2-1);
        vec K = abs(curvature_precise(res.X));
        vec dK = diff(K);

        for (int i = 1; i < subsub.n_elem-2; i+=2)
        {
            int a = subsub[i];
            int b = subsub[i+2];
            if (b-a < 3)
                continue;
            int ii = (i+1)/2;
            gfx::color(gfx::defaultColor(ii, 0.8));
            gfx::lineWidth(3);
            gfx::draw(res.traj(a, b));
            gfx::lineWidth(1.);
            vec dk = dK.subvec(a, b);
            auto brack = max_bracket(dk, tol);
            int mx = discrete_bisection(dk, brack, tol);
            if (mx < 0)
                continue;
            
            int j = mx + a;
            assert(ii < I.n_elem-1);
            I[(i+1)/2] = j;
        }

        return I;
    }

    vec curvature(const Polyline& P_)
    {
        mat P = P_.mat();
        if (P_.closed)
        {
            P = join_horiz(join_horiz(P.col(P.n_cols-1), P), P.col(0));
        }
        
        mat D = diff(P, 1, 1);
        rowvec lr = sqrt(sum(abs(D)%abs(D), 0))+1e-200;
        D.row(0) = D.row(0)/lr;
        D.row(1) = D.row(1)/lr;
        vec l = lr.t();

        int n = D.n_cols;
        vec theta = zeros(n-1);
        for (int i = 0; i < n-1; i++)
            theta(i) = angle_between(D.col(i), D.col(i+1));
        vec K = 2.*sin(theta/2) / sqrt(l.subvec(0, n-2)%l.subvec(1, n-1) + 1e-200);

        if (!P_.closed)
        {
            K = join_vert(join_vert(K.row(0), K), K.row(K.n_cols-1));
        }
        
        return K;
    }

    void adjust_partitions(SkeletalStroke* skel, const SkeletalSpine& spine, const SketchResult& res)
    {
        /// The spine is dense, so adjust partitions and spine indices to map to curvature extrema
        vec K = curvature_precise(res.X);
        vec R = abs(1./K);

        mat D = diffs(spine.points);

        // Half segments between index map 
        uvec subsub = linspace<uvec>(0, res.traj.size()-1, res.index_map.n_elem*2-1);
        uvec new_spines_inds = zeros<uvec>(res.traj.size());
            
        int a, b;
        for (int i = 1; i < subsub.n_elem-2; i+=2)
        {
            a = subsub[i];
            b = subsub[i+2];
            int si = (i+1)/2;
            for (int j = a; j < b; j++)
            {
                new_spines_inds[j] = si;
            }    
        }

        for (int j = b; j < new_spines_inds.n_elem; j++)
        {
            new_spines_inds[j] = res.index_map.n_elem-1;
        }

        // Partitions for new spine indices
        uvec partitions = zeros<uvec>(res.traj.size());
        for (int i = 0; i <  res.index_map.n_elem-1; i++)
        {
            a = res.index_map[i];
            b = res.index_map[i+1];
            for (int j = a; j < b; j++)
            {
                partitions[j] = i;
            }   
        }

        // Retrograde segments are locations where curvature radius is less than local width
        for (int i = 0; i < skel->flesh.size(); i++)
        {
            for (int j=0; j < skel->flesh[i].size(); j++)
            {
                FleshVertex &v = skel->flesh[i][j];
                if (v.spine_index < 0)
                    continue;
                if (new_spines_inds[v.spine_index] > 0 && new_spines_inds[v.spine_index] < res.index_map.n_elem-1) 
                {
                    vec b = ag::normalize(-D.col(v.spine_index-1)) + ag::normalize(D.col(v.spine_index));
                    double dd = dot(b, v.edge_normal);
                    double r = 1. / fabs(K[v.spine_index]); 
                    
                    if (dd > 0 &&  r <= std::min(spine.widths(0, v.spine_index), spine.widths(1, v.spine_index)))
                    {
                        v.retrograde = true;
                        // gfx::color(1,0,0);
                        // gfx::drawCircle(v.pos, r);
                        // gfx::color(0,0,1);
                        // gfx::drawCircle(v.pos, r2);
                        //if (dot(ag::normalize(D.col(v.spine_index)), perp(v.edge_normal)) < 0.);
                        //    gfx::drawLine(v.pos, v.pos + v.edge_normal*dd*200 );
                    }
                    else
                    {
                        v.retrograde = false;
                    }
                }
                else
                {
                    v.retrograde = false;
                }

                v.partition_index = partitions[v.spine_index];
                v.spine_index = new_spines_inds[v.spine_index];
            }
        }
        
        for (int k = 0; k < res.index_map.n_elem; k++)
        {
            gfx::color(1,0,0);
            gfx::fillCircle(spine.points[res.index_map[k]], 4);
        }
    }
    
    void render()
    {
        float w = appWidth();
        float h = appHeight();
        
        gfx::clear(0.9,1,1,1);
        
        gfx::setOrtho(w, h);
        gfx::color(0);
        gfx::lineStipple(2);
        ctr_maker.draw();
        gfx::lineStipple(0);
        
        int m = ctr_maker.pts.size();
        if(m<2)
            return;
        
        SketchResult res = sketch_lqt(Polyline(ctr_maker.pts.points, false), softie_params, false);
        
        mat traj = res.traj;
        mat T = res.X.rows(2, 3);
        mat N = T;
        for (int i = 0; i < T.n_cols; i++)
            N.col(i) = (mat({{0,-1},{1, 0}}) * T.col(i))/(norm(T.col(i))+1e-10);
        
        //N = N / join_vert(l,l);
        gfx::color(0,1,0);
        gfx::draw(traj, false);
        for (int i = 0; i < N.n_cols; i++)
        {
            //gfx::drawLine(traj.col(i), traj.col(i) + N.col(i)*70);
        }

        SkeletalSpine spine;
        Polyline spineP(traj, false); //spineP(ctr_maker.pts.points, false); //spineP(traj, false);
        switch (params.spine_type)
        {
            case 0:
                spine = SkeletalSpine::stepwise(spineP, (params.w0 + random::uniform(spineP.num_edges())*(params.w1 - params.w0))*params.w);
                break;
            case 1:
                spine = SkeletalSpine::constant(spineP, params.w * std::max(params.w0, params.w1));
                break;
            case 2:
                spine = SkeletalSpine::lerp(spineP, params.w0*params.w, params.w1*params.w);
                break;
            case 3:
                spine = SkeletalSpine::sin(spineP, params.w0*params.w, params.w1*params.w);
                break;
            case 4:
                spine = SkeletalSpine::chisel(spineP, params.w0*params.w, params.w1*params.w, params.spine_theta);
                break;
            defualt:
                assert(0);
        }
        
        SkeletalParams sk_params;
        sk_params.duplicate_joints = false;
        sk_params.unfold = params.unfold;
        
        SkeletalStroke skel = fat_path(spine, true, sk_params); //SkeletalStroke(spine, svg_flesh, bounding_rect(svg_flesh), sk_params); // fat_path(spine, true, sk_params);
        
        if (params.adjust_index_map)
            res.index_map = adjust_index_map(res, 1.*pow(10, -params.extrema_tol));

        adjust_partitions(&skel, spine, res);
        
        Chunk chunk(skel);
        
        if (softie_params.draw_partitions)
        {
            for (int i = 0; i < chunk.layering_data.partition_shapes.size(); i++)
            {
                gfx::color(gfx::defaultColor(i, 0.5));
                gfx::fill(chunk.layering_data.partition_shapes[i]);
            }
        }
        
        gfx::color(0);
        gfx::draw(chunk.shape);
        
        LayeringInput layering_input(chunk);
        
        compute_folds(&layering_input, layering_params.fold_thresh, layering_params.debug_folds); // <- only draw folding if layering disabled
        
        if (!layering_params.active)
            return;
        
        random::seed(layering_params.seed);

        if (layering_params.shuffle_z)
            layering_input.shuffle_z();

        Layering layering(layering_input,
                          layering_params.fold_thresh,
                          layering_params.normal_thresh,
                          layering_params.disambiguate_overlaps,
                          softie_params.debug_draw);
        
        gfx::color(params.fill);
        gfx::fill(layering.whole);
        
        gfx::lineWidth(layering_params.outline_thickness);
        gfx::color(0);
        gfx::draw(layering.whole);
        gfx::draw(layering.visible_outlines);
        gfx::lineWidth(1.);

         if (layering_params.draw_hidden)
        {
            gfx::lineStipple(4);
            gfx::color(0, 0.5);
            for (const segment& seg: layering.invisible_segments)
                gfx::draw_segment(seg);
            gfx::lineStipple(0);
        }

    }
};



