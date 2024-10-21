#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inTangent;
layout(location=3) in vec2 inTexCoord;

layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec2 outTexCoord;
layout(location=3) out vec4 outTangent;
layout(location=4) out vec3 outLightVec;
layout(location=5) out vec4 outShadowCoord;

#include "glsl/transform_uniform.glsl"
#include "glsl/world_uniform.glsl"

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
	gl_Position = TRANSFORMS[gl_InstanceIndex].localToClip * vec4(inPosition, 1.0);

	outPosition = mat4x3(TRANSFORMS[gl_InstanceIndex].model) * vec4(inPosition, 1.0);
	outTexCoord = inTexCoord;

	mat3 normalMatrix = mat3(transpose(TRANSFORMS[gl_InstanceIndex].modelNormal));
    outNormal = normalize(normalMatrix * inNormal);

	mat3 model = mat3(TRANSFORMS[gl_InstanceIndex].model);
	outTangent = vec4(normalize(model * inTangent.xyz), inTangent.w);

	outLightVec = normalize(scene.lightPos.xyz - inPosition);
	outShadowCoord = biasMat * scene.lightSpace * vec4(outPosition, 1.0);
}