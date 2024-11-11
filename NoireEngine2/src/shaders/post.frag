#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1, rgba32f) uniform image2D G_Color;
layout(set = 1, binding = 2, r32f) uniform image2D raytracedAOSampler;

void main()
{
    ivec2 texelCoord = ivec2(gl_FragCoord.xy); // Example coordinate
	vec4  color = imageLoad(G_Color, texelCoord);
	float ao    = imageLoad(raytracedAOSampler, texelCoord).x;

	fragColor = color * (ao == 0 ? 1 : ao);
}
