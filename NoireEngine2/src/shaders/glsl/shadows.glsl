// Note: shadows.glsl relies on a descriptor indexed shadowMaps[] array

#include "sampling.glsl"

#define ambient 0.1

//////////////////////////////////////////////////////////////////////////
// Computes PCF for directional light given 
// `uv` the shadow coords
// `currentDepth` the actual depth of the fragment without z-test
// `uvRadius` radius of PCF sampling
float PCF(vec2 uv, float currentDepth, float uvRadius, int pcfSamples, float bias, int shadowMapIndex){
	float sum = 0;
	float stp = 64 / pcfSamples;
	for (int i = 0; i < pcfSamples; i++)
	{
		float z = texture(shadowMaps[shadowMapIndex], uv + Poisson64[int(i * stp)] * uvRadius).r;
		sum += (z < (currentDepth - bias)) ? ambient : 1;
	}
	return sum / pcfSamples;
}

//////////////////////////////////////////////////////////////////////////
float FindOccluderDistance(vec2 uv, float currentDepth, float uvLightSize, float nearClip, int occluderSamples, float bias, int shadowMapIndex)
{
	int occluders = 0;
	float avgOccluderDistance = 0;
	float searchWidth = uvLightSize * (currentDepth - nearClip) / currentDepth;
	float stp = 64 / occluderSamples;

	for (int i = 0; i < occluderSamples; i++)
	{
		float z = texture(shadowMaps[shadowMapIndex], uv + Poisson64[int(i * stp)] * searchWidth).r;
		if (z < (currentDepth - bias))
		{
			occluders++;
			avgOccluderDistance += z;
		}
	}
	if (occluders > 0)
		return avgOccluderDistance / occluders;
	else
		return -1;
}

// blocker search
const float nearClip = 1;
const int occluderSamples = 32;
const int pcfSamples = 32;
const float uvLightSize = 0.1;

//////////////////////////////////////////////////////////////////////////
float PCSS(vec2 uv, float currentDepth, float bias, int shadowMapIndex)
{
	float occluderDistance = FindOccluderDistance(uv, currentDepth, uvLightSize, nearClip, occluderSamples, bias, shadowMapIndex);
	if (occluderDistance == -1)
		return 1;

	// penumbra estimation
	float penumbraWidth = (currentDepth - occluderDistance) / occluderDistance;

	// percentage-close filtering
	float uvRadius = penumbraWidth * uvLightSize * nearClip / currentDepth;
	return PCF(uv, currentDepth, uvRadius, pcfSamples, bias, shadowMapIndex);
}