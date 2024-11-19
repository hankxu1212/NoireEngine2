#ifndef _INCLUDE_PBR
#define _INCLUDE_PBR

#include "utils.glsl"

// GGX/Towbridge-Reitz normal distribution function.
float DistributionGGX(float cosLh, float roughness)
{
	float alpha2 = pow(roughness, 4);

	float denom = (cosLh * cosLh) * (alpha2 - 1.0) + 1.0;
	return alpha2 / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float SchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float GeometrySmith(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0; // remap
	float k = (r * r) / 8.0; // only for analytic lights (not IBL)
	return SchlickG1(cosLi, k) * SchlickG1(cosLo, k);
}

/** Compute reflectance , given the cosine of the angle of incidence and the
reflectance at normal incidence. */
vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
	return mix(F0, vec3(1.0), pow(1.0 - cosTheta, 5.0));
}

// variation of FresnelSchlick that takes into account roughness
vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method (IBL version).
float GeometrySmith_IBL(float cosLi, float cosLo, float roughness)
{
	float r = roughness;
	float k = (r * r) / 2.0; // Epic suggests using this roughness remapping for IBL lighting.
	return SchlickG1(cosLi, k) * SchlickG1(cosLo, k);
}

#endif