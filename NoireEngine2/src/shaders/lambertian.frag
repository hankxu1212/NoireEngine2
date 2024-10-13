#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

#include "glsl/world_uniform.glsl"
#include "glsl/utils.glsl"

vec3 n;
#include "glsl/lighting.glsl"

layout (set = 2, binding = 0) uniform sampler2D textures[];
layout (set = 3, binding = 1) uniform samplerCube lambertianIDL;

layout( push_constant ) uniform constants
{
	vec4 albedo;
	int albedoTexId;
	int normalTexId;
	float normalStrength;
	float environmentLightIntensity;
} material;

vec3 GetNormalFromMap()
{
    vec3 tangentNormal = texture(textures[material.normalTexId], inTexCoord).rgb * 2.0 - 1.0;

    vec3 Q1  = dFdx(inPosition);
    vec3 Q2  = dFdy(inPosition);
    vec2 st1 = dFdx(inTexCoord);
    vec2 st2 = dFdy(inTexCoord);

    vec3 N  = normalize(inNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
	// normal mapping
	if (material.normalTexId >= 0)
	{
		n = GetNormalFromMap();
		n.xy *= material.normalStrength;
		n = normalize(n);
	}
	else
		n = normalize(inNormal);

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