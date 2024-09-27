#version 450

#define MAX_NUM_TOTAL_LIGHTS 20

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoord;

layout(location=0) out vec4 outColor;

struct Light {
    vec4 color;
    vec4 position;
    vec4 direction;
    float intensity;
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
	n = normalize(normal);

	//hemisphere sky + directional sun:
	vec3 lightsSum = vec3(0);
	for (int i=0;i<scene.numLights;++i)
	{
		lightsSum += CalcLight(i, scene.lights[i].type);
	}

	// lightsSum = CalcLight(0, lights[0].type);

	vec3 texColor = texture(TEXTURE, texCoord).rgb;

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
	return vec3(0);
}

vec3 SpotLight(int i)
{
	return vec3(0);
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