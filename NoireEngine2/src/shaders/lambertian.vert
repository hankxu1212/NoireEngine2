#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inTangent;
layout(location=3) in vec2 inTexCoord;

layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec2 outTexCoord;
layout(location=3) out vec4 outTangent;

#include "glsl/transform_uniform.glsl"

void main() {
	gl_Position = TRANSFORMS[gl_InstanceIndex].localToClip * vec4(inPosition, 1.0);

	outPosition = mat4x3(TRANSFORMS[gl_InstanceIndex].model) * vec4(inPosition, 1.0);
	outTexCoord = inTexCoord;

	mat3 normalMatrix = mat3(transpose(TRANSFORMS[gl_InstanceIndex].modelNormal));
    outNormal = normalize(normalMatrix * inNormal);

	mat3 model = mat3(TRANSFORMS[gl_InstanceIndex].model);
	outTangent = vec4(normalize(model * inTangent.xyz), inTangent.w);
}