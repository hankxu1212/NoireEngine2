#version 450

#extension GL_EXT_nonuniform_qualifier : require

#include "glsl/utils.glsl"

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

#include "glsl/world_uniform.glsl"

layout (set = 2, binding = 0) uniform sampler2D textures[];
layout (set = 3, binding = 0) uniform samplerCube samplerCubeMap;

void main() {
    vec3 I = normalize(inPosition - vec3(scene.cameraPos));
    vec3 R = reflect(I, normalize(inNormal));

    vec3 color = texture(samplerCubeMap, R).rgb;
	color = ACES(color);
	outColor = vec4(color, 1);
}