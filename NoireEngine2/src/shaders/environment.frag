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

void main() {
	vec3 color = texture(samplerCubeMap, normalize(inNormal)).rgb;
	color = ACES(color);
	outColor = vec4(color, 1);
}