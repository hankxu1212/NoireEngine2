#version 450

layout(location=0) in vec3 Position;
layout(location=1) in vec3 Normal;
layout(location=2) in vec4 Tangent;
layout(location=3) in vec2 TexCoord;

layout(location=0) out vec3 position;
layout(location=1) out vec3 normal;
layout(location=2) out vec2 texCoord;

#include "glsl/transform_uniform.glsl"

void main() {
	gl_Position = TRANSFORMS[gl_InstanceIndex].localToClip * vec4(Position, 1.0);
	position = mat4x3(TRANSFORMS[gl_InstanceIndex].model) * vec4(Position, 1.0);
	normal = mat3(transpose(TRANSFORMS[gl_InstanceIndex].modelNormal)) * Normal;
	texCoord = TexCoord;
}