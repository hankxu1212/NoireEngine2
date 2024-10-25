// uniforms
struct DirLight
{
	mat4 lightspace;
	vec4 color;
	vec4 direction;
	float angle;
	float intensity;
	int shadowOffset;
	float shadowStrength;
};

layout(set=1, binding=1, std140) readonly buffer DirLights {
	DirLight DIR_LIGHTS[];
};

struct PointLight
{
	mat4 lightspace;
	vec4 color;
	vec4 position;
	float intensity;
	float radius;
	float limit;
	int shadowOffset;
	float shadowStrength;
};

layout(set=1, binding=2, std140) readonly buffer PointLights {
	PointLight POINT_LIGHTS[];
};

struct SpotLight
{
	mat4 lightspace;
	vec4 color;
	vec4 position;
	vec4 direction;
	float intensity;
	float radius;
	float limit;
	float fov;
	float blend;
	int shadowOffset;
	float shadowStrength;
	float nearClip;
};

layout(set=1, binding=3, std140) readonly buffer SpotLights {
	SpotLight SPOT_LIGHTS[];
};

vec3 DirLightRadiance(int i)
{
	return max(0.0, dot(n, -vec3(DIR_LIGHTS[i].direction))) * DIR_LIGHTS[i].intensity * vec3(DIR_LIGHTS[i].color);
}

float CalculateFallOff(float D, float limit, float radius)
{
    float saturated = saturate(1.0 - pow(D / limit, 4));
    float falloff = saturated * saturated / (D * D + 1.0);
	float attenuation = 1 / (D * D);
    return mix(1.0, falloff, step(radius, D)) * attenuation;
}

vec3 PointLightRadiance(int i)
{
	vec3 lightDir = -normalize(vec3(POINT_LIGHTS[i].position) - inPosition);

	float D = distance(vec3(POINT_LIGHTS[i].position), inPosition);
    
	float cosTheta = saturate(dot(n, lightDir));

	float falloff = CalculateFallOff(D, POINT_LIGHTS[i].limit, POINT_LIGHTS[i].radius);

	return cosTheta * POINT_LIGHTS[i].intensity * falloff * vec3(POINT_LIGHTS[i].color);
}

vec3 SpotLightRadiance(int i)
{
	vec3 lightDir = -normalize(vec3(SPOT_LIGHTS[i].position) - inPosition);
	vec3 spotlightDirection = normalize(vec3(SPOT_LIGHTS[i].direction));

	float D = distance(vec3(SPOT_LIGHTS[i].position), inPosition); // distance to light

	float falloff = CalculateFallOff(D, SPOT_LIGHTS[i].limit, SPOT_LIGHTS[i].radius);

	float cosTheta = saturate(dot(lightDir, spotlightDirection));
	float fov = radians(SPOT_LIGHTS[i].fov);

	// Inner and outer angles (in radians)
    float cosThetaInner = cos(fov * (1.0 - SPOT_LIGHTS[i].blend) * 0.5);
    float cosThetaOuter = cos(fov * 0.5);

    // Compute attenuation based on the angle
    float coneAttenuation = clamp((cosTheta - cosThetaOuter) / saturate(cosThetaInner - cosThetaOuter), 0.0, 1.0);

	return cosTheta * SPOT_LIGHTS[i].intensity * falloff * coneAttenuation * vec3(SPOT_LIGHTS[i].color);
}

#include "shadows.glsl"

vec3 DirectLighting()
{
	vec3 lightsSum = vec3(0);
		
	int shadowMapOffsetId = 0;

	for (int i = 0; i < scene.numLights[0]; ++i)
	{
		vec3 radiance = DirLightRadiance(i);
		if (DIR_LIGHTS[i].shadowOffset == 1)
		{
			radiance *= DirLightShadow(i, shadowMapOffsetId);
			shadowMapOffsetId += DIR_LIGHTS[i].shadowOffset;
		}
		lightsSum += radiance;
	}

	for (int i = 0; i < scene.numLights[1]; ++i)
	{
		vec3 radiance = PointLightRadiance(i);
		if (POINT_LIGHTS[i].shadowOffset == 1)
		{
			radiance *= PointLightShadow(i, shadowMapOffsetId);
			shadowMapOffsetId += POINT_LIGHTS[i].shadowOffset;
		}
		lightsSum += radiance;
	}

	for (int i = 0; i < scene.numLights[2]; ++i)
	{
		vec3 radiance = SpotLightRadiance(i);
		if (SPOT_LIGHTS[i].shadowOffset == 1)
		{
			radiance *= SpotLightShadow(i, shadowMapOffsetId);
			shadowMapOffsetId += SPOT_LIGHTS[i].shadowOffset;
		}
		lightsSum += radiance;
	}

	return lightsSum;
}