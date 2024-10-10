#include "utils.glsl"

vec2 SampleConcentricDisc(vec2 inVec) 
{
    vec2 offset = inVec * 2.0 - 1.0;
    
    if (offset == vec2(0.0))
        return vec2(0.0);

    float theta, r;

    if (abs(offset.x) > abs(offset.y)) {
        r = offset.x;
        theta = QUARTER_PI * (offset.y / offset.x);
    } else {
        r = offset.y;
        theta = HALF_PI - QUARTER_PI* (offset.x / offset.y);
    }

    return r * vec2(cos(theta), sin(theta));
}

vec3 CosineSampleHemisphere(float u1, float u2) 
{
    vec2 d = SampleConcentricDisc(vec2(u1, u2));
    float z = sqrt(max(0.0, 1.0 - d.x * d.x - d.y * d.y));
    return vec3(d.x, z, d.y);
}