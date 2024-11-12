#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1, rgba32f) uniform image2D G_Color;
layout(set = 1, binding = 2, r32f) uniform image2D raytracedAOSampler;

#include "glsl/utils.glsl"

layout(push_constant) uniform params_
{
    int useToneMapping;
};

void main()
{
    ivec2 texelCoord = ivec2(gl_FragCoord.xy); // Example coordinate
	
	vec3  color = imageLoad(G_Color, texelCoord).rgb;

	if (useToneMapping == 1)
		color = ACES(color);

	float ao    = imageLoad(raytracedAOSampler, texelCoord).x;
	fragColor = vec4(color * (ao == 0 ? 1 : ao), 1);
}
