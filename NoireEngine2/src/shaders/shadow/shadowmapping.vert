#version 450

layout (location = 0) in vec3 inPos;

layout( push_constant ) uniform constants
{
	int lightspaceID;
};

layout(set=0, binding=0, std140) readonly buffer Lightspaces {
	mat4 LIGHTSPACES[];
};

// transforms
struct Transform 
{
	mat4 localToClip;
	mat4 model;
	mat4 modelNormal;
};

layout(set=1, binding=0, std140) readonly buffer Transforms {
	Transform TRANSFORMS[];
};

void main()
{
	gl_Position = LIGHTSPACES[lightspaceID] * TRANSFORMS[gl_InstanceIndex].model * vec4(inPos, 1.0);
}