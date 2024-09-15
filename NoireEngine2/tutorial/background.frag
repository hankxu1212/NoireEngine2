#version 450

layout(location = 0) in vec2 position;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
	float time;
} pushData;

void main()
{
    outColor = vec4(0,0,0,1);
}