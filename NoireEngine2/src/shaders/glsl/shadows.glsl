// Note: shadows.glsl relies on a descriptor indexed shadowMaps[] array

#include "sampling.glsl"

#define ambient 0.1

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

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

float DirLightShadow(int i)
{
	const float shadowBias = 0.002;
	vec4 shadowCoord = biasMat * DIR_LIGHTS[i].lightspace * vec4(inPosition, 1.0);
		
	// perform perspective divide
	shadowCoord /= shadowCoord.w;

	// when z/w goes outside the normalized range [-1, 1], they lie outside the view frustum
	// similarly for x,y, they need to be in [0, 1]
	float shadow = 1;
	if (abs(shadowCoord.z) <= 1
		&& shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0
		&& shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0)
	{
		shadow = PCSS(shadowCoord.xy, shadowCoord.z, shadowBias, i);
	}
	return shadow;
}

float PointLightShadow(int i)
{
	return 1;
}

float SpotLightShadow(int i)
{
	float shadowBias = max(0.05 * (1.0 - dot(n, vec3(SPOT_LIGHTS[i].position) - inPosition)), 0.005);  
	vec4 shadowCoord = biasMat * SPOT_LIGHTS[i].lightspace * vec4(inPosition, 1.0);
	
	// perform perspective divide
	shadowCoord /= shadowCoord.w;

	// when z/w goes outside the normalized range [-1, 1], they lie outside the view frustum
	// similarly for x,y, they need to be in [0, 1]
	float shadow = 1;
	if (abs(shadowCoord.z) <= 1
		&& shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0
		&& shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0)
	{
		shadow = PCSS(shadowCoord.xy, shadowCoord.z, shadowBias, i);
	}
	return shadow;
}

float CalculateShadow()
{
	// shadow calculation
	float shadow = 1;

	for (int i = 0; i < scene.numShadowCasters[0]; ++i)
		shadow *= DirLightShadow(i);

	for (int i = 0; i < scene.numShadowCasters[1]; ++i)
		shadow *= PointLightShadow(i);

	for (int i = 0; i < scene.numShadowCasters[2]; ++i)
		shadow *= SpotLightShadow(i);

	return shadow;
}