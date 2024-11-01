// Note: shadows.glsl relies on a descriptor indexed shadowMaps[] array

#include "sampling.glsl"
#include "cubemap.glsl"

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
		sum += (z < (currentDepth - bias)) ? 0 : 1;
	}
	return sum / pcfSamples;
}

//////////////////////////////////////////////////////////////////////////
float PCSS(vec2 uv, float currentDepth, float bias, int shadowMapIndex, float lightSize)
{
	const float nearClip = 0.1;

	int occluders = 0;
	float avgOccluderDistance = 0;
	float searchWidth = lightSize * (currentDepth - nearClip) / currentDepth;
	float stp = 64 / scene.occluderSamples;

	for (int i = 0; i < scene.occluderSamples; i++)
	{
		float z = texture(shadowMaps[shadowMapIndex], uv + Poisson64[int(i * stp)] * searchWidth).r;
		if (z < (currentDepth - bias))
		{
			occluders++;
			avgOccluderDistance += z;
		}
	}
	if (occluders == 0)
		return 1;

	float occluderDistance = avgOccluderDistance / occluders;

	// penumbra estimation
	float penumbraWidth = (currentDepth - occluderDistance) / occluderDistance;

	// percentage-close filtering
	float uvRadius = penumbraWidth * lightSize * nearClip / currentDepth;
	return PCF(uv, currentDepth, uvRadius, scene.pcfSamples, bias, shadowMapIndex);
}

float DirLightShadow(int lightId, int shadowMapId)
{
	// Get cascade index for the current fragment's view position
	int cascadeIndex = 0;
	for(int i = 0; i < DIR_LIGHTS[lightId].shadowOffset - 1; ++i) {
		if(inViewPos.z < DIR_LIGHTS[lightId].splitDepths[i]) {	
			cascadeIndex = i + 1;
		}
	}

	int cascadedShadowMapID = shadowMapId + cascadeIndex;

	const float shadowBias = EPSILON;
	vec4 shadowCoord = DIR_LIGHTS[lightId].lightspaces[cascadeIndex] * vec4(inPosition, 1.0);
		
	shadowCoord /= shadowCoord.w;

	float shadow = 1;
	if (abs(shadowCoord.z) <= 1
		&& shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0
		&& shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0)
	{
		shadow = (1 - PCF(shadowCoord.xy, shadowCoord.z, 0.0005, scene.pcfSamples, 0.0005, cascadedShadowMapID)) * DIR_LIGHTS[lightId].shadowStrength;
	}
	return 1 - shadow;
}

float PointLightShadow(int lightId, int shadowMapId)
{
	vec3 lightDir = normalize(inPosition - vec3(POINT_LIGHTS[lightId].position));
	int face = CartesianToCubeFace(lightDir);

	int omniShadowMapID = shadowMapId + face;

	float shadowBias = 0.0005;

	vec4 shadowCoord = POINT_LIGHTS[lightId].lightspaces[face] * vec4(inPosition, 1.0);

	shadowCoord /= shadowCoord.w;
	
	float shadow = 1;
	if (abs(shadowCoord.z) <= 1
		&& shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0
		&& shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0)
	{
		float lightSize = POINT_LIGHTS[lightId].radius;
		shadow = (1 - PCSS(shadowCoord.xy, shadowCoord.z, shadowBias, omniShadowMapID, lightSize)) * POINT_LIGHTS[lightId].shadowStrength;
	}
	return 1 - shadow;
}

float SpotLightShadow(int lightId, int shadowMapId)
{
	float shadowBias = 0.0005;

	vec4 shadowCoord = SPOT_LIGHTS[lightId].lightspace * vec4(inPosition, 1.0);
	
	// perform perspective divide
	shadowCoord /= shadowCoord.w;

	// when z/w goes outside the normalized range [-1, 1], they lie outside the view frustum
	// similarly for x,y, they need to be in [0, 1]
	float shadow = 1;
	if (abs(shadowCoord.z) <= 1
		&& shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0
		&& shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0)
	{
		float lightSize = SPOT_LIGHTS[lightId].radius;
		shadow = (1 - PCSS(shadowCoord.xy, shadowCoord.z, shadowBias, shadowMapId, lightSize)) * SPOT_LIGHTS[lightId].shadowStrength;
	}
	return 1 - shadow;
}