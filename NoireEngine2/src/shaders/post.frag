#version 450

layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform sampler2D G_Color;
layout(set = 0, binding = 2) uniform sampler2D G_Normal;

void main()
{
	vec2  uv    = outUV;
	vec4  color = texture(G_Color, uv);
	float ao    = texture(G_Normal, uv).x;

	fragColor = color;
}
