uniform sampler2D tex;
uniform float gamma;
uniform float luma;

float HCLgamma = 3;
float HCLy0 = 100;
float HCLmaxL = 0.530454533953517f;  // == exp(HCLgamma / HCLy0) - 0.5
float PI = 3.14159265358979323846;

vec3 rgb_to_hsv(vec3 c) {
    vec4 K = vec4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    vec4 p = mix(vec4(c.b, c.g, K.w, K.z), vec4(c.g, c.b, K.x, K.y), step(c.b, c.g));
    vec4 q = mix(vec4(p.x, p.y, p.w, c.r), vec4(c.r, p.y, p.z, p.x), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10f;
    return vec3(abs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
}

vec3 hsv_to_rgb(vec3 c) {
    vec4 K = vec4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    vec3 p = abs(fract(vec3(c.x) + vec3(K)) * 6.0f - vec3(K.w));
    return c.z * mix(vec3(K.x), clamp(p - vec3(K.x), 0.0f, 1.0f), c.y);
}

vec3 hcl_to_rgb(vec3 HCL) {
    vec3 RGB = vec3(0);
    if (HCL.z != 0) {
        float H = HCL.x;
        float C = HCL.y;
        float L = HCL.z * HCLmaxL;
        float Q = exp((1 - C / (2 * L)) * (HCLgamma / HCLy0));
        float U = (2 * L - C) / (2 * Q - 1);
        float V = C / Q;
        float T = tan((H + min(fract(2 * H) / 4.f, fract(-2 * H) / 8.f)) * PI * 2);
        H *= 6;
        if (H <= 1) {
            RGB.r = 1;
            RGB.g = T / (1 + T);
        } else if (H <= 2) {
            RGB.r = (1 + T) / T;
            RGB.g = 1;
        } else if (H <= 3) {
            RGB.g = 1;
            RGB.b = 1 + T;
        } else if (H <= 4) {
            RGB.g = 1 / (1 + T);
            RGB.b = 1;
        } else if (H <= 5) {
            RGB.r = -1 / T;
            RGB.b = 1;
        } else {
            RGB.r = 1;
            RGB.b = -T;
        }
        RGB = RGB * V + U;
    }
    return RGB;
}

vec3 rgb_to_hcl(vec3 rgb) {
    vec3 HCL;
    float H = 0;
    float U = min(rgb.r, min(rgb.g, rgb.b));
    float V = max(rgb.r, max(rgb.g, rgb.b));
    float Q = HCLgamma / HCLy0;
    HCL.y = V - U;
    if (HCL.y != 0) {
        H = atan(rgb.r - rgb.g, rgb.g - rgb.b) / PI;
        Q *= U / V;
    }
    Q = exp(Q);
    HCL.x = fract(H / 2.f - min(fract(H), fract(-H)) / 6.f);
    HCL.y *= Q;
    HCL.z = mix(-U, V, Q) / (HCLmaxL * 2);
    return HCL;
}


 
void main(void)
{
    vec2 uv = gl_TexCoord[0].xy; 
    vec4 clr = texture2D(tex, uv);
    
    clr.xyz = hcl_to_rgb(clr.xyz);
    vec3 hsv = rgb_to_hsv(clr.xyz);
    hsv.z *= luma;
    clr.xyz = hsv_to_rgb(hsv);
    clr.xyz = pow(clr.xyz, vec3(1.0/gamma));
    gl_FragColor = clr;
}
