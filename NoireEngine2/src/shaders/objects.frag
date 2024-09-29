#version 450

#define MAX_NUM_TOTAL_LIGHTS 20
#define MAX_LIGHTS_PER_OBJ 8

#define saturate(x) clamp(x, 0.0, 1.0)

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

struct Light {
    vec4 color;
    vec4 position;
    vec4 direction;
	float radius;
	float limit;
	float intensity;
	float fov;
	float blend;
	int type;
};

layout(set=0,binding=0,std140) uniform World {
	Light lights[MAX_NUM_TOTAL_LIGHTS];
	int numLights;
}scene;

layout(set=2,binding=0) uniform sampler2D TEXTURE;

layout( push_constant ) uniform constants
{
	vec4 albedo;
} material;

vec3 F0 = vec3(0.04);
vec3 n;
float gamma = 2.2f;

vec3 CalcLight(int i, int type);

void main() {
	n = normalize(inNormal);

	//hemisphere sky + directional sun:
	vec3 lightsSum = vec3(0);
	for (int i = 0; i < min(MAX_LIGHTS_PER_OBJ, scene.numLights); ++i)
	{
		lightsSum += CalcLight(i, scene.lights[i].type);
	}

	vec3 texColor = texture(TEXTURE, inTexCoord).rgb;

	vec3 color = vec3(material.albedo) * texColor * lightsSum;

    // gamma correct
	color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/ gamma)); 

	outColor = vec4(color, 1);
}

vec3 DirLight(int i)
{
	return max(0.0, dot(n,vec3(scene.lights[i].direction))) * scene.lights[i].intensity * vec3(scene.lights[i].color);
}

vec3 PointLight(int i)
{
	Light light = scene.lights[i];

	float D = distance(vec3(light.position), inPosition); // distance to light
    
	vec3 lightDir = normalize(vec3(light.position) - inPosition);
	
	float cosTheta = saturate(dot(n, lightDir));

	float d_over_radius = (D / light.limit);
	float d_over_radius_4 = d_over_radius * d_over_radius * d_over_radius * d_over_radius;
	float saturated = saturate(1 - d_over_radius_4);
	float attenuation = saturated * saturated / (D * D + 1);

	return cosTheta * light.intensity * attenuation * vec3(light.color);
}

vec3 SpotLight(int i)
{
	Light light = scene.lights[i];

	float D = distance(vec3(light.position), inPosition); // distance to light

    vec3 lightDir = normalize(vec3(light.position) - inPosition);

	float cosTheta = saturate(dot(n, lightDir));

    // spotlight intensity
    float theta = dot(lightDir, vec3(normalize(light.direction)));
    float inner = cos(light.fov * (1 - light.blend) / 2);
    float outer = cos(light.fov / 2);
	float epsilon = saturate(inner - outer);
	float intensity = clamp((theta - outer) / epsilon, 0.0, 1.0);

	// not sure if this is the right way to attune limit
	float d_over_radius = (D / light.limit);
	float d_over_radius_4 = d_over_radius * d_over_radius * d_over_radius * d_over_radius;
	float saturated = saturate(1 - d_over_radius_4);
	float attenuation = saturated * saturated / (D * D + 1);

	return cosTheta * light.intensity * intensity * attenuation * vec3(light.color);
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