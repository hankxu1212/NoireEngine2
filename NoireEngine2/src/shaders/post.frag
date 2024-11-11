#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform sampler2D G_Color;
layout(set = 0, binding = 3) uniform sampler2D raytracedAOSampler;

void main()
{
	vec4  color = texture(G_Color, uv);
	float ao    = texture(raytracedAOSampler, uv).x;

	fragColor = color * (ao == 0 ? 1 : ao);
}
