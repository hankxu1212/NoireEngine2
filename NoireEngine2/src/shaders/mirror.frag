#version 450

#extension GL_EXT_nonuniform_qualifier : require

#define saturate(x) clamp(x, 0.0, 1.0)

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

#include "glsl/world_uniform.glsl"

layout (set = 2, binding = 0) uniform sampler2D textures[];
layout (set = 3, binding = 0) uniform samplerCube samplerCubeMap;

vec3 n;
float gamma = 2.2f;

vec3 CalcLight(int i, int type);

void main() {
    vec3 I = normalize(inPosition - vec3(scene.cameraPos));
    vec3 R = reflect(I, normalize(inNormal));

	vec3 sampleUVW = R;
	sampleUVW.xy *= -1;
	sampleUVW = sampleUVW.xzy;

	vec3 skyboxColor = texture(samplerCubeMap, sampleUVW).rgb;

	vec3 color = skyboxColor;

    // gamma correct
	color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/ gamma)); 

	outColor = vec4(color, 1);
}