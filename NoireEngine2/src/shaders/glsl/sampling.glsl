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

vec3 CosineSampleHemisphere(vec2 u) 
{
    return CosineSampleHemisphere(u.x, u.y);
}

// Uniformly sample point on a hemisphere.
// Cosine-weighted sampling would be a better fit for Lambertian BRDF but since this
// compute shader runs only once as a pre-processing step performance is not *that* important.
// See: "Physically Based Rendering" 2nd ed., section 13.6.1.
vec3 UniformSampleHemisphere(float u1, float u2)
{
	const float u1p = sqrt(max(0.0, 1.0 - u1*u1));
	return vec3(cos(TWO_PI*u2) * u1p, sin(TWO_PI*u2) * u1p, u1);
}

// Compute Van der Corput radical inverse
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Sample i-th point from Hammersley point set of NumSamples points total.
vec2 SampleHammersley(uint i)
{
	return vec2(i * InvNumSamples, RadicalInverse_VdC(i));
}

// Epic's UE4 IBL
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
vec3 SampleGGX(float u1, float u2, float roughness)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha*alpha - 1.0) * u2));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta); // Trig. identity
	float phi = TWO_PI * u1;

	// Convert to Cartesian upon return.
	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}