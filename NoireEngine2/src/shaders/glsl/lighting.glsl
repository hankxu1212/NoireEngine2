float DirLightRadiance(int i)
{
	vec3 lightDir = -vec3(DIR_LIGHTS[i].direction);
	float cosTheta = max(dot(n, lightDir), 0.0);

	return cosTheta * DIR_LIGHTS[i].intensity;
}

float CalculateFallOff(float D, float limit, float radius)
{
    float saturated = saturate(1.0 - pow(D / limit, 4));
    float falloff = saturated * saturated / (D * D + 1.0);
	float r = max(radius, D);
	float attenuation = 1 / (FOUR_PI * r * r);
    return mix(1.0, falloff, step(limit, D)) * attenuation;
}

float PointLightRadiance(int i)
{
	float D = distance(vec3(POINT_LIGHTS[i].position), inPosition);
	float falloff = CalculateFallOff(D, POINT_LIGHTS[i].limit, POINT_LIGHTS[i].radius);
	if (falloff <= 0)
		return 0;

	vec3 lightDir = normalize(vec3(POINT_LIGHTS[i].position) - inPosition);
    
	float cosTheta = max(dot(n, lightDir), 0.0);

	return cosTheta * POINT_LIGHTS[i].intensity * falloff;
}

float SpotLightRadiance(int i)
{
	float D = distance(vec3(SPOT_LIGHTS[i].position), inPosition);
	float falloff = CalculateFallOff(D, SPOT_LIGHTS[i].limit, SPOT_LIGHTS[i].radius);
	if (falloff <= 0)
		return 0;

	vec3 lightDir = normalize(vec3(SPOT_LIGHTS[i].position) - inPosition);
    
	float cosTheta = max(dot(n, lightDir), 0.0);

	float cosAngle = saturate(dot(lightDir, -vec3(SPOT_LIGHTS[i].direction)));
	float fov = radians(SPOT_LIGHTS[i].fov);

	// Inner and outer angles (in radians)
    float cosThetaInner = cos(fov * (1.0 - SPOT_LIGHTS[i].blend) * 0.5);
    float cosThetaOuter = cos(fov * 0.5);
	float coneAttenuation = clamp((cosAngle - cosThetaOuter) / saturate(cosThetaInner - cosThetaOuter), 0.0, 1.0);
	
	return cosTheta * SPOT_LIGHTS[i].intensity * falloff * coneAttenuation;
}

#include "shadows.glsl"

vec3 DirectLighting()
{
	vec3 lightsSum = vec3(0);
		
	int shadowMapOffsetId = 0;

	for (int i = 0; i < scene.numLights[0]; ++i)
	{
		float radiance = DirLightRadiance(i);

		if (radiance > 0 && DIR_LIGHTS[i].shadowOffset > 0)
			radiance *= DirLightShadow(i, shadowMapOffsetId);

		shadowMapOffsetId += DIR_LIGHTS[i].shadowOffset;
		lightsSum += radiance * vec3(DIR_LIGHTS[i].color);
	}

	for (int i = 0; i < scene.numLights[1]; ++i)
	{
		float radiance = PointLightRadiance(i);
		
		if (radiance > 0 && POINT_LIGHTS[i].shadowOffset > 0)
			radiance *= PointLightShadow(i, shadowMapOffsetId);
		
		shadowMapOffsetId += POINT_LIGHTS[i].shadowOffset;
		lightsSum += radiance * vec3(POINT_LIGHTS[i].color);
	}

	for (int i = 0; i < scene.numLights[2]; ++i)
	{
		float radiance = SpotLightRadiance(i);

		if (radiance > 0 && SPOT_LIGHTS[i].shadowOffset > 0)
			radiance *= SpotLightShadow(i, shadowMapOffsetId);

		shadowMapOffsetId += SPOT_LIGHTS[i].shadowOffset;
		lightsSum += radiance * vec3(SPOT_LIGHTS[i].color);
	}

	return lightsSum;
}