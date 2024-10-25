#version 450

layout (location = 0) in vec3 inPos;

layout( push_constant ) uniform constants
{
	int lightspaceID;
};

layout(set=0, binding=0, std140) readonly buffer Lightspaces {
	mat4 LIGHTSPACES[];
};

#include "../glsl/transform_uniform.glsl"

void main()
{
	gl_Position = LIGHTSPACES[lightspaceID] * TRANSFORMS[gl_InstanceIndex].model * vec4(inPos, 1.0);
}