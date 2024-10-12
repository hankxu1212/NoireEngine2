// you need to define these functions to use a forward lighting pass
vec3 LIGHTING_VAR_NORMAL;
mat3 LIGHTING_VAR_TBN;
vec3 LIGHTING_VAR_TANGENT_FRAG_POS;
vec3 LIGHTING_VAR_TANGENT_VIEW_POS;

vec3 DirLight(int i);
vec3 PointLight(int i);
vec3 SpotLight(int i);
vec3 CalcLight(int i, int type);

#define CURR_LIGHT scene.lights[i]

vec3 DirectLighting()
{
	vec3 lightsSum = vec3(0);
	for (int i = 0; i < min(MAX_LIGHTS_PER_OBJ, scene.numLights); ++i)
	{
		lightsSum += CalcLight(i, CURR_LIGHT.type);
	}
	return lightsSum;
}

vec3 DirLight(int i)
{
	return max(0.0, dot(LIGHTING_VAR_NORMAL,vec3(CURR_LIGHT.direction))) * CURR_LIGHT.intensity * vec3(CURR_LIGHT.color);
}

vec3 PointLight(int i)
{
	vec3 tangentLightPos = LIGHTING_VAR_TBN * vec3(CURR_LIGHT.position);
	vec3 lightDir = normalize(tangentLightPos - LIGHTING_VAR_TANGENT_FRAG_POS);
	// vec3 viewDir = normalize(LIGHTING_VAR_TANGENT_VIEW_POS - LIGHTING_VAR_TANGENT_FRAG_POS);

	float D = distance(vec3(CURR_LIGHT.position), inPosition); // distance to light
    
	float cosTheta = saturate(dot(LIGHTING_VAR_NORMAL, lightDir));

	float saturated = saturate(1 - pow(D / CURR_LIGHT.limit, 4));
	float attenuation = saturated * saturated / (D * D + 1);

	return cosTheta * CURR_LIGHT.intensity * attenuation * vec3(CURR_LIGHT.color);
}

vec3 SpotLight(int i)
{
	vec3 tangentLightPos = LIGHTING_VAR_TBN * vec3(CURR_LIGHT.position);
	vec3 lightDir = normalize(tangentLightPos - LIGHTING_VAR_TANGENT_FRAG_POS);
	// vec3 viewDir = normalize(LIGHTING_VAR_TANGENT_VIEW_POS - LIGHTING_VAR_TANGENT_FRAG_POS);

	float D = distance(vec3(CURR_LIGHT.position), inPosition); // distance to light
    
	float cosTheta = saturate(dot(LIGHTING_VAR_NORMAL, lightDir));

	float saturated = saturate(1 - pow(D / CURR_LIGHT.limit, 4));
	float attenuation = saturated * saturated / (D * D + 1);

    // spotlight intensity
    float theta = dot(lightDir, vec3(normalize(CURR_LIGHT.direction)));
    float inner = cos(CURR_LIGHT.fov * (1 - CURR_LIGHT.blend) / 2);
    float outer = cos(CURR_LIGHT.fov / 2);
	float epsilon = saturate(inner - outer);
	float intensity = clamp((theta - outer) / epsilon, 0.0, 1.0);

	return cosTheta * CURR_LIGHT.intensity * intensity * attenuation * vec3(CURR_LIGHT.color);
}

vec3 CalcLight(int i, int type)
{
	if (type == 0) // dir light
		return DirLight(i);
	else if (type == 1) // point light
		return PointLight(i);
	else // spot light
		return SpotLight(i);
}