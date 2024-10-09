#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

#include "glsl/world_uniform.glsl"
#include "glsl/utils.glsl"

layout (set = 2, binding = 0) uniform sampler2D textures[];
layout (set = 3, binding = 0) uniform samplerCube samplerCubeMap;

vec3 n;
float gamma = 2.2f;

void main() {
	outColor = gamma_map(vec3(texture(samplerCubeMap, inNormal)), gamma);
}