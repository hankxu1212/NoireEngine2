#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec3 inTangent;
layout(location=4) in vec3 inBitangent;

layout(location=0) out vec4 outColor;

#include "glsl/world_uniform.glsl"
#include "glsl/utils.glsl"
#include "glsl/lighting.glsl"

layout (set = 2, binding = 0) uniform sampler2D textures[];
layout (set = 3, binding = 1) uniform samplerCube lambertianIDL;

layout( push_constant ) uniform constants
{
	vec4 albedo;
	int albedoTexId;
	int normalTexId;
	int roughnessTexId;
	int metallicTexId;
	float roughness;
	float metallic;
	float normalStrength;
	float environmentLightIntensity;
} material;

vec3 F0 = vec3(0.04);
vec3 n;
mat3 TBN;

void main() {
	// normal mapping
	if (material.normalTexId >= 0)
	{
		n = texture(textures[material.normalTexId], inTexCoord).rgb;
		n = normalize(n * 2.0 - 1.0);  // this normal is in tangent space
		n.xy *= material.normalStrength;
		n = normalize(n);
	}
	else
		n = normalize(inNormal);

	vec3 t = normalize(inTangent);
	vec3 b = normalize(inBitangent);
	TBN = mat3(t, b, n);

	LIGHTING_VAR_NORMAL = n;
	LIGHTING_VAR_TBN = TBN;
	LIGHTING_VAR_TANGENT_FRAG_POS = TBN * inPosition;
	LIGHTING_VAR_TANGENT_VIEW_POS = TBN * vec3(scene.cameraPos);

	//hemisphere sky + directional sun:
	vec3 lightsSum = DirectLighting();

	// IDL
	vec3 ambientLighting;
	{
		vec3 environmentalLight = vec3(texture(lambertianIDL, n));
		ambientLighting = environmentalLight * vec3(material.albedo);
		ambientLighting *= material.environmentLightIntensity;
	}

	// material
	vec3 texColor = vec3(1);
	if (material.albedoTexId >= 0)
		texColor = texture(textures[material.albedoTexId], inTexCoord).rgb;

	vec3 color = vec3(material.albedo) * texColor * lightsSum + ambientLighting;

	outColor = gamma_map(color, 2.2f);
}