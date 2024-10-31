#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec4 inTangent;
layout(location=4) in vec3 inViewPos;

layout(location=0) out vec4 outColor;

layout (set = 2, binding = 0) uniform sampler2D textures[]; // global texture array, indexed
layout (set = 3, binding = 1) uniform samplerCube lambertianIDL;
layout (set = 4, binding = 0) uniform sampler2D shadowMaps[]; // shadow maps, indexed

#include "glsl/world_uniform.glsl"
#include "glsl/utils.glsl"

vec3 n;
#include "glsl/lighting.glsl"

layout( push_constant ) uniform constants
{
	vec4 albedo;
	int albedoTexId;
	int normalTexId;
	float normalStrength;
	float environmentLightIntensity;
} material;

void main() 
{
    // calculate TBN
    n = normalize(inNormal);
    vec3 tangent = inTangent.xyz;
    tangent = normalize(tangent - dot(tangent, n) * n);
    vec3 bitangent = normalize(cross(n, inTangent.xyz) * inTangent.w);
    mat3 TBN = mat3(tangent, bitangent, n);

	// normal mapping
    // Perform sampling before (potentially) discarding.
	// This is to avoid implicit derivatives in non-uniform control flow.
	if (material.normalTexId >= 0)
	{
        n = 2.0 * texture(textures[material.normalTexId], inTexCoord).rgb - 1.0;
		n.xy *= material.normalStrength;
		n = normalize(n);
        n = normalize(TBN * n);
	}

	// sample diffuse irradiance
	vec3 ambientLighting = texture(lambertianIDL, n).rgb * material.environmentLightIntensity;

	// material
	vec3 texColor = material.albedo.rgb;
	if (material.albedoTexId >= 0)
		texColor *= texture(textures[material.albedoTexId], inTexCoord).rgb;

	// direct lighting and shadows
	vec3 directLighting = DirectLighting();

	vec3 color = texColor * (directLighting + ambientLighting);
	
	color = ACES(color);
	outColor = vec4(color, 1);
}