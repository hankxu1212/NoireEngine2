#version 450

layout(location=0) in vec3 inPosition;
layout(location=0) out vec3 outFragUVW;

layout(set=0, binding=0, std140) uniform Camera {
	mat4 projection;
	mat4 view;
};

void main()
{
	outFragUVW = inPosition;
	outFragUVW.xy *= -1.0;

	vec4 pos = projection * mat4(mat3(view)) * vec4(inPosition, 1.0);

	gl_Position = pos.xyww;
}