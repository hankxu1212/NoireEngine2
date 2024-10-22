#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 depthMVP;
} ubo;

void main()
{
	gl_Position =  ubo.depthMVP * vec4(inPos, 1.0);
}