//------------------------------------------------------------------------------------
// A lot of the functions adapted from iq.
// http://www.iquilezles.org/
// https://www.shadertoy.com/user/iq




#define MAX_CUBES 100

uniform vec2 resolution; // screen resolution
uniform float time; // current time
uniform vec2 mouse; // mouse position (screen space)

// uniform vec3 box_pos, box_rot, box_scale;  // for testing individual transforms
// uniform mat4 box_mat;    // for testing whole transform
uniform vec3 box_pos[MAX_CUBES];
uniform vec3 box_dir[MAX_CUBES];
uniform vec3 box_hole[MAX_CUBES];

uniform float thresh;

uniform int num_cubes;

uniform mat4 invViewMatrix;
uniform float tanHalfFov; // tan(fov/2)

uniform float blend_k;

uniform float fog_amt;
uniform vec4 diffuse_color;
uniform vec4 background_color;
uniform vec4 light_color;
uniform float light_intensity;

uniform sampler2D tex_0;

const float EPSILON = 0.01;
const float PI = 3.1415926535;
const float PI2 = PI*2.0;

float radians( float x )
{
    return PI/180.*x;
}

uniform vec3 light;

// Modify these functions
float compute_scene( in vec3 p, out int mtl );
vec3 object_color( in vec3 p, in vec3 rd, in float distance, in int mtl);
//vec3 object_color(in in int mtl, in vec3 p, in vec3 rd);

//------------------------------------------------------------------------------------
#pragma mark NOISE
// SIMPLE NOISE
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Simplex Noise (http://en.wikipedia.org/wiki/Simplex_noise), a type of gradient noise
// that uses N+1 vertices for random gradient interpolation instead of 2^N as in regular
// latice based Gradient Noise.

vec2 hash( vec2 p )
{
    p = vec2( dot(p,vec2(127.1,311.7)),
             dot(p,vec2(269.5,183.3)) );
    
    return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise_2d( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;
    
    vec2 i = floor( p + (p.x+p.y)*K1 );
    
    vec2 a = p - i + (i.x+i.y)*K2;
    vec2 o = (a.x>a.y) ? vec2(1.0,0.0) : vec2(0.0,1.0); //vec2 of = 0.5 + 0.5*vec2(sign(a.x-a.y), sign(a.y-a.x));
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0*K2;
    
    vec3 h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
    
    vec3 n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    
    return dot( n, vec3(70.0) );
}



//------------------------------------------------------------------------------------
#pragma mark UTILS
float saturate( in float v )
{
    return clamp(v,0.0,1.0);
}

float expose( in float l, in float e )
{
    return (1.5 - exp(-l*e));
}

const vec4 lumi = vec4(0.30, 0.59, 0.11, 0);

float luminosity( in vec4 clr )
{
    return dot(clr, lumi);
}

vec4  normal_color( in vec3 n )
{
    return vec4((n*vec3(0.5)+vec3(0.5)), 1);
}

float attenuation( in float distance, in float atten )
{
    return min( 1.0/(atten*distance*distance), 1.0 );
}

//// Smooth blend functions
////  http://www.iquilezles.org/www/articles/smin/smin.htm
float smin_exp( float a, float b, float k )
{
    float res = exp( -k*a ) + exp( -k*b );
    return -log( res )/k;
}

float smin_poly( float a, float b, float k )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}


// power smooth min (k = 8);
float smin_power( float a, float b, float k )
{
    a = pow( a, k ); b = pow( b, k );
    return pow( (a*b)/(a+b), 1.0/k );
}


//------------------------------------------------------------------------------------
#pragma mark SDF PRIMITIVES
// SDF Objects
// p: sample position
// assumes object is at 0, 0, 0

//f(x,z) = sin(x)·sin(z)
//color = pow( color, vec3(1.0/2.2) );
float sdf_xz_plane(in vec3 p, float y)
{
    return p.y - y;//+ sin(p.x*1.0)*sin(p.z*1.0)*0.9 - y; // + sin(p.x*3.0)*sin(p.z*2.0)*0.3
}

float sdf_xy_plane(in vec3 p, float z)
{
    return p.z - z;//+ sin(p.x*1.0)*sin(p.z*1.0)*0.9 - y; // + sin(p.x*3.0)*sin(p.z*2.0)*0.3
}

float sdf_yz_plane(in vec3 p, float x)
{
    return p.x - x;//+ sin(p.x*1.0)*sin(p.z*1.0)*0.9 - y; // + sin(p.x*3.0)*sin(p.z*2.0)*0.3
}

float sdf_box(in vec3 p, in vec3 size)
{
    vec3 d = abs(p) - size;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdf_round_box(in vec3 p, in vec3 size, float smoothness )
{
    return length(max(abs(p)-size*0.5,0.0))-smoothness;
}

float sdf_sphere(in vec3 p, in float radius)
{
    return length(p)-radius;
}

float sdf_torus(in vec3 p, in float radius, in float thickness )
{
    vec2 q = vec2(length(p.xz)-radius,p.y);
    return length(q)-thickness;
}

float sdf_prism( in vec3 p, in vec2 h )
{
    vec3 q = abs(p);
    return max(q.z-h.y,max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5);
}


float sdf_torus( in vec3 p, in vec2 t )
{
    return length( vec2(length(p.xz)-t.x,p.y) )-t.y;
}

float sdf_hex_prism( in vec3 p, in vec2 h )
{
    vec3 q = abs(p);
#if 1
    return max(q.z-h.y,max((q.x*0.866025+q.y*0.5),q.y)-h.x);
#else
    float d1 = q.z-h.y;
    float d2 = max((q.x*0.866025+q.y*0.5),q.y)-h.x;
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
#endif
}

float sdf_capsule( in vec3 p, in vec3 a, in vec3 b, in float r )
{
    vec3 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

float sdf_cylinder( in vec3 p, in vec2 h )
{
    vec2 d = abs(vec2(length(p.xz),p.y)) - h;
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sdf_cone( in vec3 p, in vec3 c )
{
    vec2 q = vec2( length(p.xz), p.y );
#if 0
    return max( max( dot(q,c.xy), p.y), -p.y-c.z );
#else
    float d1 = -p.y-c.z;
    float d2 = max( dot(q,c.xy), p.y);
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
#endif    
}


//------------------------------------------------------------------------------------
#pragma mark SDF OPERATORS

float sdf_union(in float d1, in float d2)
{
    return min(d1, d2);
}

float sdf_subtract(in float d1, in float d2)
{
    return max(-d2, d1);
}

float sdf_intersect(in float d1, in float d2)
{
    return max(d1, d2);
}

float sdf_blend_exp( in float d1, in float d2, in float k )
{
    return smin_exp(d1, d2, k);
}

float sdf_blend_poly( in float d1, in float d2, in float k )
{
    return smin_poly(d1, d2, k);
}

float sdf_blend_power( in float d1, in float d2, in float k )
{
    return smin_power(d1, d2, k);
}

/*
 float sdf_blend(vec3 p, float a, float b)
 {
 float s = smoothstep(length(p), 0.0, 1.0);
 float d = mix(a, b, s);
 return d;
 }
 */

vec3 sdf_repeat( in vec3 p, in vec3 rep)
{
    vec3 d = mod(p, rep) - 0.5*rep;
    return d;
}

vec3 sdf_translate( in vec3 p, in vec3 offset )
{
    return p-offset;
}


vec3 sdf_rotate_y(in vec3 p, float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    vec3 res;
    res.x = p.x * c - p.z * s;
    res.y = p.y;
    res.z = p.x * s + p.z * c;
    return res;
}

vec3 sdf_rotate_x(in vec3 p, float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    vec3 res;
    res.x = p.x;
    res.y = p.y * c - p.z * s;
    res.z = p.y * s + p.z * c;
    return res;
}

vec3 sdf_rotate_z(in vec3 p, float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    vec3 res;
    res.x = p.x * c - p.y * s;
    res.y = p.x * s + p.y * c;
    res.z = p.z;
    return res;
}

/// We actually pass in the inverse transformation here because it would be slow to do it in the shader.
vec3 sdf_transform(in vec3 p, in mat4 inv_mat)
{
    return (inv_mat*vec4(p,1.0)).xyz;
}

vec3 sdf_scale(in vec3 p, in vec3 scale) {
    return p / scale;
}



//------------------------------------------------------------------------------------
#pragma mark LIGHTING

//---------------------------------------------------
// from iq. https://www.shadertoy.com/view/Xds3zN
vec3 calc_normal( in vec3 p )
{
    //vec3 delta = vec3( 0.0001, 0.0, 0.0 );
    vec3 delta = vec3( 2./resolution.x, 0.0, 0.0 )*1;
    int mtl;
    vec3 n;
    n.x = compute_scene( p+delta.xyz, mtl ) - compute_scene( p-delta.xyz, mtl );
    n.y = compute_scene( p+delta.yxz, mtl ) - compute_scene( p-delta.yxz, mtl );
    n.z = compute_scene( p+delta.yzx, mtl ) - compute_scene( p-delta.yzx, mtl );
    n = normalize(n);
    //return vec3(1.,0.,0.);
    //if (length(n) < 1)
    //    return vec3(1.,0.,0.);//normalize(vec3(1.,0.,1.));
    return n;
    //return normalize( n );
}


vec3 calc_normal_( in vec3  p ) // for function f(p)
{
    int mtl;
    const float h = 0.001*10; // or some other value
    const vec2 k = vec2(1,-1);
    return normalize( k.xyy*compute_scene( p + k.xyy*h, mtl ) + 
                      k.yyx*compute_scene( p + k.yyx*h, mtl ) + 
                      k.yxy*compute_scene( p + k.yxy*h, mtl ) + 
                      k.xxx*compute_scene( p + k.xxx*h, mtl ) );
}


//---------------------------------------------------
#define ambient_occlusion ambient_occlusion3

// from iq. https://www.shadertoy.com/view/Xds3zN
float ambient_occlusion3( in vec3 pos, in vec3 nor )
{
    float occ = 0.001;
    float sca = 0.9;
    int mtl;
    int niter=9;
    for( int i=0; i<niter; i++ )
    {
        float hr = 0.001 + 0.06*float(i)/niter;
        vec3 aopos =  nor * hr + pos;
        float dd = compute_scene( aopos, mtl );
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 4.0*occ, 0.0, 1.0 );
}


//---------------------------------------------------
float ambient_occlusion2( in vec3 p, vec3 n ) //, float stepDistance, float samples)
{
    const float stepDistance = 0.25;//EPSILON;
    float samples = 4.0;
    float occlusion = 0.2;
    int mtl;
    for (occlusion = 1.0 ; samples > 0.0 ; samples-=1.0) {
        occlusion -= (samples * stepDistance - (compute_scene( p + n * samples * stepDistance, mtl))) / pow(2.0, samples);
    }
    return occlusion;
}

//---------------------------------------------------
float ambient_occlusion1( in vec3 p, in vec3 n ) //, float startweight, float diminishweight )
{
    float startweight=1.;
    float diminishweight=0.3;

    //n = vec3(0.0,1.0,1.0);
    float ao = 0.2;
    float weight = startweight;
    int mtl;
    
    for ( int i = 1; i < 6; ++i )
    {
        float delta = i*i*EPSILON *12.0;
        ao += weight * (delta-compute_scene(p+n*(0.0+delta), mtl));
        weight *= diminishweight;
    }
    
    return 1.0-saturate(ao);
}

float ambient_occlusion4(in vec3 p, in vec3 n ){
	float sum = 0.0;
    
    const float ao_samples = 4.0;
    const float ao_spacing = 0.4;
    const float ao_strength = 3.0;
     int mtl;

	for(float i=1.0; i<=ao_samples; i++){
		float d = i*ao_spacing;
		sum += (d - compute_scene(p + n*d, mtl))/pow(2.0, i);
	}
	
	return 1.0 - ao_strength*sum;
}



//---------------------------------------------------
//#define soft_shadow     soft_shadow1

// from iq. https://www.shadertoy.com/view/Xds3zN
float soft_shadow2( in vec3 ro, in vec3 rd, in float mint, in float tmax, float k )
{
    float res = 1.0;
    float t = mint;
    int mtl;
    for( int i=0; i<76; i++ )
    {
        float h = compute_scene( ro + rd*t, mtl );
        res = min( res, k*h/t );
        t += h*0.5;//clamp( h, 0.02, 0.10 );
        if( h<0.001 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );
}


float soft_shadow1( in vec3 p, in vec3 w, float mint, float maxt, float k )
{
    float res = 1.0;
    int mtl;
    for( float t=mint; t < maxt; )
    {
        float h = compute_scene(p + w*t,mtl);
        if( h<0.001 )
            return 0.0;
        res = min( res, k*h/t );
        t += h * 1.0;
    }
    return res;
}

float hard_shadow(in vec3 ro, in vec3 rd, float mint, float maxt) {
    int mtl;
    for(float t=mint; t < maxt;) {
        float h = compute_scene(ro + rd*t, mtl);
        if(h<0.001) return 0.0;
        t += h;
    }
    return 1.0;
}

float soft_shadow( in vec3 ro, in vec3 rd, in float mint, in float tmax, int technique )
{
    int mtl;
	float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<32; i++ )
    {
		float h = compute_scene( ro + rd*t, mtl );

        // traditional technique
        if( technique==0 )
        {
        	res = min( res, 10.0*h/t );
        }
        // improved technique
        else
        {
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0 : h*h/(2.0*ph); 

            float y = h*h/(2.0*ph);
            float d = sqrt(h*h-y*y);
            
            res = min( res, 10.0*d/max(0.0,t-y) );
            ph = h;
            
        }
        
        t += h;
        
        if( res<0.0001 || t>tmax ) break;
        
    }
    return clamp( res, 0.0, 1.0 );
}





//------------------------------------------------------------------------------------
#pragma mark RAY MARCHER

//#define compute_color compute_color_pass
vec3 light_dir(vec3 p)
{
    return normalize(light); //mat3(invViewMatrix) * normalize(light); // - p);
}

vec4 compute_color_outline( in vec3 p, in float distance, in int mtl, in float normItCount )
{
    return vec4(1.);
}


float lambert( in vec3 l, in vec3 n )
{
    float nl = dot(n, l);
	
    return max(0.0,nl);
}

float oren_nayar( in vec3 l, in vec3 n, in vec3 v, float r )
{
    float r2 = r*r;
    float a = 1.0 - 0.5*(r2/(r2+0.57));
    float b = 0.45*(r2/(r2+0.09));

    float nl = dot(n, l);
    float nv = dot(n, v);

    float ga = dot(v-n*nv,n-n*nl);

	return max(0.0,nl) * (a + b*max(0.0,ga) * sqrt((1.0-nv*nv)*(1.0-nl*nl)) / max(nl, nv));
}

vec4 compute_lighting( in vec3 p, in float distance, in int mtl, in float normItCount )
{
    //return vec4(1.,0.,0.,1.);
    //return vec4(abs(p)/10., 1.);
    vec4 it_clr = vec4(vec3(0.1+normItCount), 1.0) * 2.0;
    //return it_clr;
    //return vec4(distance*100000000);
    vec3 n = calc_normal(p);
    vec3 l = normalize( vec3( 0.6, 0.5, 0.4) );
    //vec3 l = normalize(vec3(-10.,-12,-34) - p); ///normalize(vec3(2.6,4,6.7));
    float nl = max(lambert(l, n), luminosity(normal_color(n))*1.3);//, 0.0); // max(0.4, dot(n, l));
    //return vec4(nl) * hard_shadow(p, l, 0.1, 10.);
    //return vec4(nl) * soft_shadow(p, l, 0.03, 4., 0) * ambient_occlusion3(p, n);
    //return normal_color(n);
    return vec4(nl) * ambient_occlusion1(p, n);

    //return vec4(max(0.5,luminosity(normal_color(n))) * max(0.3,ambient_occlusion1(p, n)*1.3)); // use this to debug normals
    return vec4(1. - nl*hard_shadow(p, light_dir(p), 0.1, 10.));
    return vec4(max(0.5,luminosity(normal_color(n))) 
    * max(0.7,hard_shadow(p, light_dir(p), 0.3, 10.)*1.3)); // use this to debug normals
}

vec4 compute_lighting( in vec3 p, in vec3 rd, in float distance, in int mtl, in float normItCount )
{
// shading/lighting	
    //vec3 lig = normalize( vec3( 0.6, 0.5, 0.4) );
	vec3 lig = normalize( vec3( 1.6, 0.6, 1.4) );
	
    vec3 bac = normalize( vec3(-0.6, 0.0,-0.4) );
	vec3 bou = normalize( vec3( 0.0,-1.0, 0.0) );
    vec3 nor = calc_normal(p);
    //return normal_color(nor);
    //return vec4(dot(nor, lig));

	vec3 col = vec3(1.,0.3,1.);
	if( distance<200.0 )
	{
	    //pos = ro + tmin*rd;
		
        // shadows
		//float sha = 1.0;
		//sha *= ssSphere( pos, lig, sph1 );
		//sha *= ssSphere( pos, lig, sph2 );
        float sha = soft_shadow1(p, lig, 0.01, 5., 8.);//1);
        //float sha = soft_shadow(p, lig, 0.01, 15., 1);//1);
        float occ = ambient_occlusion(p, nor);
        //col = vec3(occ);

		vec3 lin = vec3(0.0);
		
		// integrate irradiance with brdf times visibility
		vec3 diffColor = diffuse_color.xyz; //vec3(1., 0.9, 0.9)*0.3;
		if(false) //obj>1.5 )
		{
            lin += vec3(0.5,0.7,1.0)*diffColor*occ;
	        lin += vec3(5.0,4.5,4.0)*diffColor*oren_nayar( lig, nor, -rd, 1.0 )*sha;
	        lin += vec3(1.5,1.5,1.5)*diffColor*oren_nayar( bac, nor, -rd, 1.0 )*occ;
	        lin += vec3(1.0,1.0,1.0)*diffColor*oren_nayar( bou, nor, -rd, 1.0 )*occ;
		}
		else
		{
            //lin += vec3(0.5,0.7,1.0)*diffColor*occ;
            lin += vec3(0.7,0.7,0.7)*diffColor*occ;
	        lin += vec3(5.0,4.5,7.0)*diffColor*lambert( lig, nor )*sha*occ;
	        lin += vec3(1.5,1.5,1.5)*diffColor*lambert( bac, nor )*occ;
			lin += vec3(1.0,1.0,1.0)*0.8*diffColor*lambert( bou, nor )*occ;
		}

		col = lin;
		
		// participating media
		col = mix( col, vec3(0.93), 1.0-exp( -0.003*distance*distance ) );
	}
	
    // gamma	
	col = pow( col, vec3(0.45) );
	//color = pow( color, vec3(1.0/2.2) );

	return vec4( col, 1.0 );
}


vec4 compute_lighting_outdoor( in vec3 p, in vec3 rd, in float distance, in int mtl, in float normItCount )
{
    vec3 col = object_color(p, rd, distance, mtl);

	//vec3 sunDir = normalize( vec3( 0.3, 1, 2) );
	vec3 sunDir = normalize( vec3( 0.3, -1, 2) );
	
    vec3 nor = calc_normal(p + rd*1e-7);
    //return normal_color(nor);
    //vec3 lig = normalize( vec3( 0.6, 0.5, 0.4) );
    //float sha = soft_shadow(p, sunDir, 0.01, 9., 1);//1);
    //float sha = hard_shadow(p, sunDir, 0.01, 5.);//, 1.);//1);
    float sha = soft_shadow1(p, sunDir, 0.05, 1.6, 50.);//1);
    float occ =  ambient_occlusion(p, nor);

    float sun = clamp( dot( nor, sunDir ), 0.0, 1.0 );
    float sky = clamp( 0.5 + 0.5*nor.y, 0.0, 1.0 );
    float ind = clamp( dot( nor, normalize(sunDir*vec3(-1.0,0.0,-1.0)) ), 0.0, 1.0 );

    // compute lighting
    vec3 lin  = sun*light_color.xyz*light_intensity*pow(vec3(sha*occ),vec3(0.9725, 0.9569, 0.102));
         lin += sky*vec3(0.5294, 0.9098, 0.9608)*occ;
         lin += ind*vec3(0.2941, 0.7843, 0.9059)*occ;

    // multiply lighting and materials
    col = col * lin;

    // gamma	
	col = pow( col, vec3(0.45) );
	//color = pow( color, vec3(1.0/2.2) );
    // participating media
    col = mix( background_color.xyz, col, exp( -fog_amt*distance*distance ) );
	
	return vec4( col, 1.0 );
}

vec3 fusion(float x) {
	float t = clamp(x,0.0,1.0);
	return clamp(vec3(sqrt(t), t*t*t, max(sin(PI*1.75*t), pow(t, 12.0))), 0.0, 1.0);
}

// HDR version
vec3 fusionHDR(float x) {
	float t = clamp(x,0.0,1.0);
	return fusion(sqrt(t))*(0.5+2.*t);
}


vec3 distanceMeter(float dist, float rayLength, vec3 rayDir, float camHeight) {
    float idealGridDistance = 20.0/rayLength*pow(abs(rayDir.y),0.8);
    float nearestBase = floor(log(idealGridDistance)/log(10.));
    float relativeDist = abs(dist/camHeight);
    
    float largerDistance = pow(10.0,nearestBase+1.);
    float smallerDistance = pow(10.0,nearestBase);

   
    vec3 col = fusionHDR(log(1.+relativeDist));
    col = max(vec3(0.),col);
    if (sign(dist) < 0.) {
        col = col.grb*3.;
    }

    float l0 = (pow(0.5+0.5*cos(dist*PI*2.*smallerDistance),10.0));
    float l1 = (pow(0.5+0.5*cos(dist*PI*2.*largerDistance),10.0));
    
    float x = fract(log(idealGridDistance)/log(10.));
    l0 = mix(l0,0.,smoothstep(0.5,1.0,x));
    l1 = mix(0.,l1,smoothstep(0.0,0.5,x));

    col.rgb *= 0.1+0.9*(1.-l0)*(1.-l1);
    return col;
}

// s = df(p);
//         s *= (s>so?2.:1.);so=s; // Enhanced Sphere Tracing => lgdv.cs.fau.de/get/2234 
// 		t += s * 0.2;
// 		p = ro + rd * t;

// Ray marcher

vec4 trace_ray_enhanced(in vec3 p, in vec3 w, in vec4 bg_clr, in float pixelRadius )
{
    int mtl=0;

    float t_min = 1e-6;
    float t_max = 30.;//distance;
    
    float t = t_min;
    
    float omega = 1.1;
    float candidate_error = 1e13;
    float candidate_t = t_min;
    float previousRadius = 0.;
    float stepLength = 0.;
    float functionSign = compute_scene(p, mtl) < 0. ? -1. : 1.;
    
    for (int i = 0; i < 512; ++i) {
        float d = compute_scene(p + t*w, mtl);
        float signedRadius = functionSign * d;
        float radius = abs(signedRadius);

        bool sorFail = omega > 1. &&
        (radius + previousRadius) < stepLength;
        if (sorFail) {
            stepLength -= omega * stepLength;
            omega = 1.;
        } else {
            stepLength = signedRadius * omega;
        }

        previousRadius = radius;
        float error = radius / t;

        if (!sorFail && error < candidate_error) {
            candidate_t = t;
            candidate_error = error;
        }

        if (!sorFail && error < pixelRadius || t > t_max) break;

        t += stepLength;
   	}

    if ( t > t_max || candidate_error > pixelRadius )
        return bg_clr;

    return compute_lighting_outdoor(p + t*w, w, t, mtl, 0.);//float(i) * 1.0/float(maxIterations));//+vec3(float(i)/128.0);
}


vec4 trace_ray(in vec3 p, in vec3 w, in vec4 bg_clr, inout float distance )
{
    //    const float maxDistance = 50;//1e10;
    const int maxIterations = 512;
    const float closeEnough = 1e-4;
    vec3 rp = p;
    int mtl=0;
    float t = 0;
    float so = 1.;;

    float d = 1.;
    for (int i = 0; i < maxIterations; ++i)
    {
        rp = p+w*t;
        d = compute_scene(rp, mtl);
        float dplane = sdf_xy_plane(rp, 0.0);
        t += d*0.5;
        //t += d*1;
        

        //vec3 clr = vec3(d*0.3);
        if (d < closeEnough)
        {
            distance = t;
            
            //vec3 clr = distanceMeter(d, t, w, 0.3);
            
            // use this to debug number of ray casts
            //return vec4(vec3(float(i)/128.0), 1.0);
//            return mtl == 0 ? vec4(vec3(float(i)/128.0), 1.0) : compute_color(rp,t,mtl);
            vec4 clr = compute_lighting_outdoor(rp, w, t, mtl, float(i) * 1.0/float(maxIterations));//+vec3(float(i)/128.0);
            return clr;
            //return vec4(clr,1.); //mix(bg_clr, clr, exp(-distance*distance*fog_amt*0.1));
        }
        else if(t > distance)
        {
            return bg_clr;//vec3(0.0);
        }
        
        
    }
    
    return bg_clr; //vec4(1.);//bg_clr;//vec3(0.0); // return skybox here
}




//------------------------------------------------------------------------------------
#pragma mark SCENE

#define blending sdf_blend_poly



//------------------------------------------------------------------------------------

float sdf_box_texture( in vec3 p, in vec3 size, in sampler2D tex )
{
    vec4 clr = texture2D(tex,(p.xz*0.1)+vec2(0.5));
    return sdf_box((p-vec3(0.0,-(clr.r)*333,0.0)),size);
}

// float compute_scene_( in vec3 p, out int mtl )
// {
//     mtl = 0;
//     float d = 1e10;
    
//     d = sdf_union(d, terrain(p));//sdf_xz_plane(p, texture2D(floor_image,p.xz*0.01).x*14.0-20.0));//sin(p.x*0.3)*sin(p.z*0.1)-20.0));//noise(p.xz) * 5.0) );
// //    float d2 = sdf_box_texture( p,vec3(6.0),floor_image );
//     float d2 = sdf_box( p,vec3(6.0) );
//     if(d2<d)
//         mtl = 1;
//     return min(d,d2);
// }

float sdf_replace(float d, float d2)
{
    return d2;
}

float make_form( float d, in vec3 p)
{
    for( int i = 0; i < num_cubes; i++ )
    {
         //d = blending(d, sdf_box(p + box_pos_dir[i].xyz, vec3(0.3)), blend_k);// vec3(0.3)));
         d = sdf_union(d, sdf_box(p + box_pos[i], box_dir[i]/2));// vec3(0.3)));
         //d = blending(d, sdf_box(p + box_pos[i], box_dir[i]/2), blend_k);// vec3(0.3)));

         //d = blending(d, sdf_sphere(p + box_pos_dir[i].xyz, 0.3), blend_k);// vec3(0.3)));
    }   

    for( int i = 0; i < num_cubes; i++ )
    {
         //d = blending(d, sdf_box(p + box_pos_dir[i].xyz, vec3(0.3)), blend_k);// vec3(0.3)));
         d = sdf_subtract(d, sdf_box(p + box_pos[i], box_hole[i]/2));// vec3(0.3)));
         //d = blending(d, sdf_box(p + box_pos[i], box_hole[i]/2), blend_k*0.3);// vec3(0.3)));
         //d = blending(d, sdf_sphere(p + box_pos_dir[i].xyz, 0.3), blend_k);// vec3(0.3)));
    }  

    return d;
}


float onion( in float sdf, in float thickness )
{
    return abs(sdf)-thickness;
}

float smin_cubic( float a, float b, float k )
{
    return min(a, b);
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*h*k*(1.0/6.0);
}

float extrusion( in vec3 p, in float sdf, in float h )
{
    vec2 w = vec2( sdf, abs(p.z) - h );
    //eturn smoothstep(0,  min(max(w.x,w.y),0.0), )
  	return smin_cubic(max(w.x,w.y),-0.001, 0.1) + length(max(w, 0.0));
}

float sd2_cross( in vec2 p, in vec2 b, float r ) 
{
    p = abs(p); p = (p.y>p.x) ? p.yx : p.xy;
    
	vec2  q = p - b;
    float k = max(q.y,q.x);
    vec2  w = (k>0.0) ? q : vec2(b.y-p.x,-k);
    
    return sign(k)*length(max(w,0.0)) + r;
}

float sdf_rounded( in float d, in float rad )
{
    return d - rad;
}

vec3 sym_x( in vec3 p )
{
    p.x = abs(p.x);
    return p;
}

vec3 sym_xz( in vec3 p )
{
    p.xz = abs(p.yx);
    return p;
}




vec3 twist_z( in vec3 p, in float k, in float b )
{
    float c = cos(k*p.z);
    float s = sin(k*p.z+b);
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xy,p.z);
    return q;
}

//funky
// vec3 twist_y( in vec3 p, in float k, in float b )
// {
//     float c = cos(k*p.y);
//     float s = sin(k*p.y*b);
//     mat2  m = mat2(c,-s,s,c);
//     vec3  q = vec3(m*p.xz,p.x);
//     return q;
// }

//also funky
vec3 twist_y( in vec3 p, in float k, in float b )
{
    float c = cos(k*p.y);
    float s = sin(k*p.y+b);
    mat2  m = mat2(c,-s,s,c);
    vec2 pp = m*p.xz;
    return vec3(pp.x, p.y, pp.y);
    //vec3  q = vec3(m*p.yx,p.z);
    //return q;
}

vec3 twist_x( in vec3 p, in float k, in float b )
{
    float c = cos(k*p.x);
    float s = sin(k*p.x+b);
    mat2  m = mat2(c,-s,s,c);
    vec2 pp = m*p.yz;
    return vec3(p.x, pp);//p.y, pp.y);
    //vec3  q = vec3(m*p.yx,p.z);
    //return q;
}

float triangle_wave(in float x, in float k)
{
    k = 1. / k;
    return (k - abs(mod(x, 2.*k) - k))/(k*0.5) - 1;
}

float triangle_cos(in float x)
{
    return triangle_wave(x, PI/10.);
}

float triangle_sin(in float x)
{
    return -triangle_wave(x + PI/2., PI/10.);
}


vec3 triangle_twist_z_( in vec3 p, in float k, in float b )
{
    float c = triangle_wave(p.z*k, 1)*0.5; //abs(cos(k*p.z));
    float s = triangle_wave(p.z*k + b, 1)*0.5; //abs(sin(k*p.z*b));
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xy,p.z);
    return q;
}

vec3 triangle_twist_z( in vec3 p, in float k, in float b )
{
    float th1 = triangle_wave(p.z*k, 5); 
    float th2 = triangle_wave(p.z*k + b, 5); 
    
    float c = cos(th1); //abs(cos(k*p.z));
    float s = sin(th2); //triangle_wave(p.x*3 - b, -k); //abs(sin(k*p.z*b));
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xy,p.z);
    return q;
}

uniform float twist_k;
uniform float twist_b;
uniform float extr;
uniform float radius;

float sdf_quad_extrude( in vec3 p, in float size, in float amount )
{
    vec2 d = abs(p.xy)-vec2(size, size);//0.5,0.5);
    float l = length(max(d, vec2(0))) + min(max(d.x,d.y),0.0);
    l = extrusion( p, l, amount);
    return l;
}

float sdf_circle_extrude( in vec3 p, in float r, in float amount )
{
    float l = length(p.xy) - r; //abs(p.xy)-vec2(size, size);//0.5,0.5);
    l = extrusion( p, l, amount);
    return l;
}

float sdf_texture_extrude( in vec3 p, in sampler2D tex, in float amount, out int mtl )
{
    vec4 clr = texture2D(tex, vec2(-p.x, p.y)+0.5);
    float l = ((clr.r-0.5)*2) / radius - 0.0;
    l *= 1.;
    l = extrusion( p, l, amount);
    //float lb = sdf_circle_extrude(p, 1.5, amount);
    
    float lb = sdf_quad_extrude(p, 0.5, amount);
    mtl = 0;
    // return l;
    // if (abs(p.z) < amount)
    // {
    //     mtl=1;
    //     if (abs(p.z) < amount && abs(p.z) > amount*0.9*(sin(time*0.1)*0.5+0.5))
    //         mtl = 100;
    // }
    // return l;
    //return lb;
    //mtl = 0;
    float lres = lb;
    if (l >= lb) //lb+0.002)
    {
        lres = l;
        //mtl = 1;
    }

    return lres;
    //return (l > lb)?l:lb;
}



// float sdf_2d_texture( in vec2 p, in sampler2D tex )
// {
//     vec4 clr = vec4(0.);
//     float o = 0.01;
//     clr += texture2D(tex, p.xy + 0.5) ; // (texture2D(tex, p.xy + 0.5) - 0.5)*2;
//     // clr += texture2D(tex, p.xy + 0.5 + vec2(-o,-o)); // (texture2D(tex, p.xy + 0.5) - 0.5)*2;
//     // clr += texture2D(tex, p.xy + 0.5 + vec2( o,-o)); // (texture2D(tex, p.xy + 0.5) - 0.5)*2;
//     // clr += texture2D(tex, p.xy + 0.5 + vec2( o, o)); // (texture2D(tex, p.xy + 0.5) - 0.5)*2;
//     // clr += texture2D(tex, p.xy + 0.5 + vec2(-o, o)); // (texture2D(tex, p.xy + 0.5) - 0.5)*2;
//     // clr /= 5.;
//     float bl = 0.5;
//     vec2 d = abs(p)-vec2(bl, bl);//0.5,0.5);
//     float l = length(max(d, vec2(0))) + min(max(d.x,d.y),0.0);
//     //l = length(p) - 0.5;

//     //return l;//
//     float smoothing = 0.19;
//     float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, clr.r)/radius;
//     alpha = ((clr.r-0.5)*2) / radius - 0.0;
//     //float u_buffer = 0.0;
//     //float u_gamma = 0.2;
//     //float alpha = clr.r / 30; //smoothstep(u_buffer - u_gamma, u_buffer + u_gamma, clr.r);
//     return (alpha > l)?alpha:l;

//     if (l < 0.02)
//         l = max(l, alpha);
//     return max(0, l);
//     return max(0, alpha);

//     // return max(0.0001, clr.r/30 - 0.0) - 0.001;
//     // float u_buffer = 0.;
//     // float u_gamma = 0.01;
//     // float alpha = smoothstep(u_buffer - u_gamma, u_buffer + u_gamma, clr.r);
//     // return max(0, alpha);
//     // //l = max(l, clr.r);

    
//     return l;
//     // float d = (clr.r);
//     // return length(p) - d; //min(length(p),d);
// }

// float sdf_2d_texture( in vec2 p, in sampler2D tex )
// {
//     float sz=0.01;
//     return (sdf_2d_texture_sample(p, tex) + 
//             sdf_2d_texture_sample(p + vec2(-sz,-sz), tex) + 
//             sdf_2d_texture_sample(p + vec2( sz,-sz), tex) + 
//             sdf_2d_texture_sample(p + vec2( sz, sz), tex) + 
//             sdf_2d_texture_sample(p + vec2(-sz, sz), tex))/5;                        
             
             
// }


vec3 bend( in vec3 p, float k, float h )
{
     // or some other amount
     //p = p + vec3(0.2,0.,0.);
    p = p + vec3(-0.9,0.,0.)*h;
    float th = sin(p.x*k ); //triangle_wave(p.x, k);  //sin(p.x*k )*1.0;//
    float c = cos(th);
    float s = sin(th);
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(p.y, m*p.xz);
    return q;
}

vec2 lissajous(in float t, in float omega, in float delta)
{
    return vec2(cos(omega*t + delta), sin(t));
}

float random (in vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

float hashy(float n) { return fract(sin(n) * 1e4); }
float hashy(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(float x) {
	float i = floor(x);
	float f = fract(x);
	float u = f * f * (3.0 - 2.0 * f);
	return mix(hashy(i), hashy(i + 1.0), u);
}

float noise(vec2 x) {
	vec2 i = floor(x);
	vec2 f = fract(x);

	// Four corners in 2D of a tile
	float a = hashy(i);
	float b = hashy(i + vec2(1.0, 0.0));
	float c = hashy(i + vec2(0.0, 1.0));
	float d = hashy(i + vec2(1.0, 1.0));

	// Simple 2D lerp using smoothstep envelope between the values.
	// return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
	//			mix(c, d, smoothstep(0.0, 1.0, f.x)),
	//			smoothstep(0.0, 1.0, f.y)));

	// Same code, with the clamps in smoothstep and common subexpressions
	// optimized away.
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}


#define NUM_OCTAVES 5

float fbm ( in float x ) 
{
    float lacunarity=2.;
    float gain=0.5;
    float amp=0.5;
    float freq=0.3;
    
    float y = 0.;
    //Loop of octaves
    for (int i = 0; i < NUM_OCTAVES; i++) {
        y += amp * noise(freq*x);
        freq *= lacunarity;
        amp *= gain;
    }
    return y;
}

float sine_noise( in float x, in float phase )
{
    float amplitude = 2.;
    float frequency = 0.4;

    float y = 0.;//sin(x * frequency);

    y += sin(x*frequency*2.1 + phase)*4.5;
    y += sin(x*frequency*1.72 + phase*1.121)*4.0;
    y += sin(x*frequency*2.221 + phase*0.237)*5.0;
    y += sin(x*frequency*3.1122+ phase*4.269)*2.5;
    y *= amplitude*0.06;
    return y;
}

// float twisty_shape(in vec3 q, in float ex, out int mtl)
// {
//     float d = 1e10;
//     const float speed=3.1;
//     float t = time*5; // 3.;
//     //q = triangle_twist_z(q, (sin(time*speed)+1. + 2)*twist_k, time*speed*0.3);//-twist_b);
//     //vec2 lxy = lissajous(t, 10., 16.);
//     //vec2 lxy = lissajous(t, 2.1333, 35.3333);
//     float freq = 2;
//     vec2 lxy = lissajous(t, freq, freq*9.222);
//     //q = triangle_twist_z(q, (sin(t*speed)+1. + 2)*twist_k, t*speed*0.3);//-twist_b);
//     //q = twist_z(q, (sin(t*speed)+1. + 2)*twist_k, t*speed*0.3);//-twist_b);
//     //q = bend(q, lxy.x*lxy.y*2.5, ex);//sin(t*speed*6.7)*7.5);
//     //q = triangle_twist_z(q, (lxy.x+lxy.y)*twist_k, t*speed*0.3);//-twist_b);
    
//     //q = triangle_twist_z(q, fbm(t)*twist_k, t*speed*0.3);//-twist_b);
//     q = triangle_twist_z(q, sine_noise(t*5, 9.)*twist_k, fbm(t*0.5112)*twist_b);//sine_noise(t*1.7, 2.)*1);//-twist_b);
//     // techno
//     ///q = triangle_twist_z(q, (tan(time*speed)+1. + 0.6)*twist_k, time*speed*0.3);//-twist_b);
//     //q = bend(q, lxy.x*lxy.y*9.6, ex);//sin(t*speed*6.7)*7.5);
//     q = bend(q, sine_noise(-t*0.9, 12.)*9, ex);
//     q = sdf_scale(q, vec3(1.));//*sine_noise(t, 1.));
//     //q = twist_z(q, twist_k, 0.2);
    
//     //q = sdf_scale(q, vec3(3.5));
//     //q = sdf_repeat(q, vec3(2.5,2.5,0.));
    
//     //d = min( d, onion(onion( sdf_box(q, vec3(1.)), 0.3 ), 0.2) );

//     //d = min(d,extrusion( q, sd2_cross( q.xy, vec2(0.8,0.35), 0.2 ), 1. ));
//     //float extr = 0.4;
//     d = min(d, sdf_texture_extrude(q, tex_0, ex, mtl)-0.001);
//     //d = min(d,extrusion( q, sdf_2d_texture( q.xy, tex_0 ), ex ) - 0.);
//     return d;
// }

vec3 bend_tri_x( in vec3 p, float k, float phase )
{
     // or some other amount
     //p = p + vec3(0.2,0.,0.);
    //p = p + vec3(-0.9,0.,0.)*h;
    //float th = sin(p.x*k ); //triangle_wave(p.x, k);  //sin(p.x*k )*1.0;//
    float th = triangle_sin(p.x*k + phase)+1;
    float c = triangle_cos(th*PI);
    float s = triangle_sin(th*PI);
    mat2  m = mat2(c,-s,s,c);
    vec2 pp = m*p.yz;
    //vec3 q = vec3(pp.x, p.y, pp.y);
    //vec3 q = vec3(pp, p.z);
    vec3 q = vec3(p.x, pp);
    //vec3  q = vec3(m*p.xy, p.z);
    return q;
}

vec3 bend_square_x( in vec3 p, float k, float phase )
{
     // or some other amount
     //p = p + vec3(0.2,0.,0.);
    //p = p + vec3(-0.9,0.,0.)*h;
    //float th = sin(p.x*k ); //triangle_wave(p.x, k);  //sin(p.x*k )*1.0;//
    //float th = sign(sin(p.y*k + phase)); //+1;
    float ss = sin(p.z*k*1);
    float th = (ss > 0.) ? 1.:-1.;
    float amt = phase*0.9;
    float c = cos(th*PI*amt);
    float s = sin(th*PI*amt);
    mat2  m = mat2(c,-s,s,c);
    vec2 pp = m*p.yz;
    //vec3 q = vec3(pp.x, p.y, pp.y);
    //vec3 q = vec3(pp, p.z);
    vec3 q = vec3(p.x, pp);
    //vec3  q = vec3(m*p.xy, p.z);
    return q;
}

vec3 walk_bend_1(in vec3 q, in float t, in float tilt)
{
    // Walking behaviour
    q = twist_z(q, sin(t)*twist_k*2.2, 0.);//time);
    q = twist_y(q, cos(t)*twist_k*0.6,0.);// time*2);
    //q = 
    return q;
}

float twisty_shape_2(in vec3 q, in float ex, out int mtl)
{
    float d = 1e10;
    //int mtl=0;
    //q = triangle_twist_z(q, sin(time), 0.0);//sine_noise(t*1.7, 2.)*1);//-twist_b);
    //q = bend_tri_x(q, twist_k*0.9, time*0.1);// sine_noise(time*0.1, 10)*9);

    //q = bend_square_x(q, twist_k*3, sine_noise(time*0.02, 3.)*9);//fbm(time*0.1)*9);
    
    //q = twist_z(q, sin(time)*twist_k*2.2, 0.);//time);
    //q = twist_y(q, cos(time)*twist_k*0.6,0.);// time*2);
    //q = twist_x(q, twist_k*0.2,0.);// time*0.5);
    //q = triangle_twist_z(q, twist_k, -twist_b);
    //q = sdf_scale(q, vec3(3.5));
    //q = sdf_repeat(q, vec3(2.5,2.5,0.));
    
    //d = min( d, onion(onion( sdf_box(q, vec3(1.)), 0.3 ), 0.2) );

    //d = min(d,extrusion( q, sd2_cross( q.xy, vec2(0.8,0.35), 0.2 ), 1. ));
    //float extr = 0.4;
    //d = min(d, extrusion( q, sdf_2d_texture( q.xy, tex_0 ), ex ));
    //return sdf_sphere(q, 0.5);
    //q = sdf_rotate_y(q, radians(100));
    //q = bend_square_x(q, twist_k*3, sine_noise(time*0.1, 3.)*2);//fbm(time*0.1)*9);
    
    
    d = min(d, sdf_texture_extrude(q, tex_0, ex, mtl));
    return d;
}

void set_material(float d,  float min_d,  int mtl, inout int out_mtl)
{
    if (d < min_d)
        out_mtl = mtl;
}


float rounded_squares_texture(in vec3 p)
{
    float div = 1.9;
    float v = 0.0;
    v = (fract(p.x*div)-0.5)*(fract(p.z*div)-0.5);
    v = saturate(pow(v*220,2.9));
    return max(0.2,v);
}

vec3 transform_camera(in vec3 p)
{
    vec3 q = p;
    //
    float speed = 0.3;
    
    //q = sdf_rotate_z(q, sin(time*1.)*0.1);
    q = sdf_translate(q, vec3(-time*speed - (sin(time*2)*0.5+0.5)*0.2*speed, 0, 0));
    return q;

}

vec3 object_color( in vec3 p, in vec3 rd, in float distance, in int mtl)
{
    if (mtl==0)
        return diffuse_color.xyz;
    else if (mtl==1)
        return vec3(0.4627, 0.749, 0.9137);
    else if (mtl==2)
        return vec3(0.0, 0.349, 1.0)*rounded_squares_texture(transform_camera(p));
    return vec3(0.0, 0.0, 0.0);
}

float split_sphere( in vec3 p, float r, int iter )
{
    vec3 q = p;
    float d = 1e10;
    for (int i = 0; i < iter; i++)
    {
        vec3 pq = floor(q*5);
        vec3 q1 = sdf_translate(q, vec3(noise(pq.x + time*0.7), noise(pq.z+time), noise(pq.z))*0.3);
        vec3 q2 = sdf_translate(q, vec3(noise(-pq.x*3. + time), noise(pq.z*2.), -noise(pq.z))*0.3);

        float d1 = sdf_box(q1, vec3(r));
        float d2 = sdf_box(q2, vec3(r));
        //return d1;
        // vec3 plane = normalize(vec3(noise(pq.x+time)*3, noise(pq.y-time*0.3)*10, noise(-pq.z)));
        // float side1 = dot(p, plane );
        // float side2 = dot(p, -plane);
        //d1 = max(d1, side1);
        //d2 = max(d2, side2);
        vec3 q = q1;
        if (d2 < d1)
            q = q2;
        d = min(d1, d2);// min(d, min(d1, d2));
    }

    return d; 
}

float compute_scene( in vec3 p, out int mtl )
{
    mtl = 0;
    float min_d = 1e10;
    //float blend_k = 1.;
    //d = sdf_union(d, sdf_xz_plane(p, sin(p.x*0.3)*sin(p.z*0.1)));//noise(p.xz) * 5.0) );
//    d = sdf_union(d, sdf_xz_plane(p,  0));
    vec3 q = p;
    //q = triangle_twist_z(q, twist_k, twist_b);
    //q = twist_z(q, twist_k*2, twist_b);
    float drep = 0.9;
    //q = sdf_rotate_z(q, time*-1/6);//transform_camera(q);
    q = sdf_repeat(q, vec3(drep,drep,0.));
    //q = sdf_rotate_x(q, time*1.0);
    
    q = sdf_translate(q, vec3(0.,0.,sin(time*2)*0.035));
    // cool effect:
    //q = twist_z(q, twist_k, twist_b); // - vec3(-3.0,0.0,1.0);q
    float d = 1e10;
    int obj_mtl = 0;
    for (int i = 0; i < 1; i++)
    {
        //d = twisty_shape_2(q, extr, obj_mtl);
        
        vec3 dq = q;
        //dq = sdf_rotate_z(dq, time*0.3); //-sin(time*1.0)*3);
        //dq = sdf_rotate_z(dq, -sin(time*1.0)*3);

        dq = sdf_translate(walk_bend_1(dq, time, 10.), vec3(0.,0.,0.));
                //vec3(sin(time*0.3 + i*0.1123)*0.0, noise(time*0.2+0.3*i)*0.1,0.)*2.3);

        dq = sdf_rotate_z(dq, radians(sin(time)*90*i));
    
        dq = sdf_rotate_y(dq, radians(90+5)); 

        d = min(d, twisty_shape_2(dq, extr, obj_mtl));
        //d = sdf_blend_poly(d, twisty_shape_2(dq, extr, obj_mtl), 0.01);

    }

    //d = split_sphere(q, extr*4, 1);
    // cut it all in half so we can see the interiors
    //d = max( d, dot(p, normalize(vec3(sin(time),-cos(time)*0.2,cos(time)))));//p.y );

    set_material(d, min_d, obj_mtl, mtl);
    min_d = min(d, min_d);
    float floord = twisty_shape_2(vec3(1.,2.,extr), extr, obj_mtl);//, extr, obj_mtl);
    
    d = sdf_xy_plane(p, -0.35);
    set_material(d, min_d, 2, mtl);
    min_d = min(d, min_d);
    //min_d = sdf_blend_poly(d, min_d, 0.15);
    //mtl = 0;
    //d = smin_cubic(d, sdf_xy_plane(p, -extr-0.02), 3.01);
    //d = sdf_rounded(d, 0.01);
    
    return min_d;// + texture2D(floor_image, p.xz * floor_scale).r * floor_height - floor_offset;
}


//------------------------------------------------------------------------------------
#pragma mark MAIN
void main(void)
{
    vec2 xy = gl_FragCoord.xy; 
    
    // Primary ray origin
    vec3 p = invViewMatrix[3].xyz;
    // Primary ray direction
    vec3 w = mat3(invViewMatrix) * normalize(
                                             vec3( (xy - resolution / 2.0)*vec2(1.0,1.0), resolution.y/(-2.0*tanHalfFov))
                                             );
      
    float distance = 1e3;
    float dist = texture2D(tex_0, xy/resolution).r;
    float u_buffer = 0.;
    float u_gamma = 0.2;
    float alpha = smoothstep(u_buffer - u_gamma, u_buffer + u_gamma, dist);
    //vec4 clr = vec4(alpha);
    //texture2D(tex,(p.xz*0.1)+vec2(0.5));
    //vec4 clr = texture2D(tex_0, xy*0.001);
    vec4 clr = trace_ray_enhanced(p, w, background_color, (0.1/resolution.y)*sqrt(2));
    //vec4 clr = trace_ray(p, w, background_color, distance);

    //clr = vec4(xy.x/resolution.x, xy.y/resolution.y, 0., 1.);
    //clr.xyz = pow( clr.xyz, vec3(1.0/2.2)); // gamma correction.
    //clr = vec4(abs(p)/50,1.);
    //clr.xyz = texture2D(color_image, vec2(luminosity(clr), 0.0)).xyz;
    
    //clr.w  = 1.0;
    gl_FragColor = clr;
}



