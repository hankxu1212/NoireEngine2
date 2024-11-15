#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inTangent;
layout(location=3) in vec2 inTexCoord;

layout(location=0) out vec3 outPosition;

struct Transform 
{
	mat4 localToClip;
	mat4 model;
	mat4 modelNormal;
};

layout(set=0, binding=1, std140) readonly buffer Transforms {
	Transform TRANSFORMS[];
};

void main() {
	gl_Position = TRANSFORMS[gl_InstanceIndex].localToClip * vec4(inPosition, 1.0);
	
	outPosition = mat4x3(TRANSFORMS[gl_InstanceIndex].model) * vec4(inPosition, 1.0);
}