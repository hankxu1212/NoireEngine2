// uniforms
struct DirLight
{
	vec4 color;
	vec4 direction;
	float angle;
	float intensity;
};

layout(set=1, binding=1, std140) readonly buffer DirLights {
	DirLight DIR_LIGHTS[];
};

struct PointLight
{
	vec4 color;
	vec4 position;
	float intensity;
	float radius;
	float limit;
};

layout(set=1, binding=2, std140) readonly buffer PointLights {
	PointLight POINT_LIGHTS[];
};

struct SpotLight
{
	vec4 color;
	vec4 position;
	vec4 direction;
	float intensity;
	float radius;
	float limit;
	float fov;
	float blend;
};

layout(set=1, binding=3, std140) readonly buffer SpotLights {
	SpotLight SPOT_LIGHTS[];
};

vec3 CalcDirLight(int i)
{
	return max(0.0, dot(n, vec3(DIR_LIGHTS[i].direction))) * DIR_LIGHTS[i].intensity * vec3(DIR_LIGHTS[i].color);
}

vec3 CalcPointLight(int i)
{
	vec3 lightDir = normalize(vec3(POINT_LIGHTS[i].position) - inPosition);

	float D = distance(vec3(POINT_LIGHTS[i].position), inPosition); // distance to light
    
	float cosTheta = saturate(dot(n, lightDir));

	float saturated = saturate(1 - pow(D / POINT_LIGHTS[i].limit, 4));
	float attenuation = saturated * saturated / (D * D + 1);

	return cosTheta * POINT_LIGHTS[i].intensity * attenuation * vec3(POINT_LIGHTS[i].color);
}

vec3 CalcSpotLight(int i)
{
	vec3 lightDir = normalize(vec3(SPOT_LIGHTS[i].position) - inPosition);

	float D = distance(vec3(SPOT_LIGHTS[i].position), inPosition); // distance to light
    
	float cosTheta = saturate(dot(n, lightDir));

	float saturated = saturate(1 - pow(D / SPOT_LIGHTS[i].limit, 4));
	float attenuation = saturated * saturated / (D * D + 1);

    // spotlight intensity
    float theta = dot(lightDir, vec3(normalize(SPOT_LIGHTS[i].direction)));
    float inner = cos(SPOT_LIGHTS[i].fov * (1 - SPOT_LIGHTS[i].blend) / 2);
    float outer = cos(SPOT_LIGHTS[i].fov / 2);
	float epsilon = saturate(inner - outer);
	float falloff = clamp((theta - outer) / epsilon, 0.0, 1.0);

	return cosTheta * SPOT_LIGHTS[i].intensity * falloff * attenuation * vec3(SPOT_LIGHTS[i].color);
}

vec3 DirectLighting()
{
	vec3 lightsSum = vec3(0);

	for (int i = 0; i < scene.numDirLights; ++i)
		lightsSum += CalcDirLight(i);

	for (int i = 0; i < scene.numPointLights; ++i)
		lightsSum += CalcPointLight(i);

	for (int i = 0; i < scene.numSpotLights; ++i)
		lightsSum += CalcSpotLight(i);

	return lightsSum;
}

vec3 CalcPBRDirectLighting(vec3 radiance, vec3 Li)
{
	// Incident light direction.
    vec3 Lh = normalize(Li + V);
    
    // Cosines of angles between normal and light vectors.
    float cosLi = max(0.0, dot(n, Li));
    float cosLh = max(0.0, dot(n, Lh));
    
    vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, V)), roughness);
    float D = DistributionGGX(cosLh, roughness);
    float G = GeometrySmith(cosLi, cosLo, roughness);
    
    // Diffuse BRDF based on Fresnel and metalness.
    vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);
    vec3 diffuseBRDF = kd * albedo;
    
    // Cook-Torrance specular BRDF.
    vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
    
    return (diffuseBRDF + specularBRDF) * radiance * cosLi;
}

vec3 DirectLightingPBR()
{
	vec3 lightsSum = vec3(0);

	for (int i = 0; i < scene.numDirLights; ++i)
	{
		vec3 radiance = CalcDirLight(i);
		lightsSum += CalcPBRDirectLighting(radiance, vec3(DIR_LIGHTS[i].direction));
	}

	for (int i = 0; i < scene.numPointLights; ++i)
	{
		vec3 radiance = CalcPointLight(i);
		vec3 Li = normalize(vec3(POINT_LIGHTS[i].position) - inPosition);
		lightsSum += CalcPBRDirectLighting(radiance, Li);
	}

	for (int i = 0; i < scene.numSpotLights; ++i)
	{
		vec3 radiance = CalcSpotLight(i);
		vec3 Li = normalize(vec3(SPOT_LIGHTS[i].position) - inPosition);
		lightsSum += CalcPBRDirectLighting(radiance, Li);
	}

	return lightsSum;
}
