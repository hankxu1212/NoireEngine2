// UE4 inspired Lambetrtian diffuse BRDF + Cook-Torrance microfacet specular BRDF + IBL

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

// Constant normal incidence Fresnel factor for all dielectrics.
#include "glsl/pbr.glsl"
#include "glsl/lighting.glsl"

layout (set = 2, binding = 0) uniform sampler2D textures[];

// IDL
#define GGX_MIP_LEVELS 6
layout (set = 3, binding = 0) uniform samplerCube skybox;
layout (set = 3, binding = 1) uniform samplerCube diffuseIrradiance;
layout (set = 3, binding = 2) uniform sampler2D specularBRDF;
layout (set = 3, binding = 3) uniform samplerCube prefilterEnvMap;

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

const vec3 Fdielectric = vec3(0.04);
vec3 n;

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

	// albedo 
	vec3 albedo = vec3(1);
	if (material.albedoTexId >= 0)
		albedo = texture(textures[material.albedoTexId], inTexCoord).rgb;
	albedo *= vec3(material.albedo);

	LIGHTING_VAR_NORMAL = n;
	LIGHTING_VAR_TBN = mat3(t, b, n);
	LIGHTING_VAR_TANGENT_FRAG_POS = LIGHTING_VAR_TBN * inPosition;

	// light out
    vec3 Lo = normalize(vec3(scene.cameraPos) - LIGHTING_VAR_TANGENT_FRAG_POS);
    float cosLo = max(0.0, dot(n, Lo));

    // specular reflection
    vec3 R = 2.0 * cosLo * n - Lo;

    float metalness = material.metallic;
    float roughness = material.roughness;

    // Fresnel
    vec3 F0 = mix(Fdielectric, albedo, metalness);

    vec3 directLighting = vec3(0);
    for (int i = 0; i < min(MAX_LIGHTS_PER_OBJ, scene.numLights); ++i)
    {
        vec3 radiance = CalcLight(i, CURR_LIGHT.type);
        
        // Incident light direction.
        vec3 Li = normalize(vec3(CURR_LIGHT.position) - LIGHTING_VAR_TANGENT_FRAG_POS);
        vec3 Lh = normalize(Li + Lo);

        // Cosines of angles between normal and light vectors.
        float cosLi = max(0.0, dot(n, Li));
        float cosLh = max(0.0, dot(n, Lh));

        vec3 F = FresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
        float D = DistributionGGX(cosLh, roughness);
        float G = GeometrySmith(cosLi, cosLo, roughness);

        // Diffuse BRDF based on Fresnel and metalness.
        vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);
        vec3 diffuseBRDF = kd * albedo;

        // Cook-Torrance specular BRDF.
        vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);

        directLighting += (diffuseBRDF + specularBRDF) * radiance * cosLi;
    }

    // IBL: Ambient lighting calculation.
    vec3 ambientLighting;
    {
        // lambertian diffuse irradiance
        vec3 irradiance = texture(diffuseIrradiance, n).rgb;

        vec3 F = FresnelSchlick(F0, cosLo);

        // Diffuse contribution for ambient light
        vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);
        vec3 diffuseIBL = kd * albedo * irradiance;

        // Specular IBL from pre-filtered environment map
        int specularTextureLevels = textureQueryLevels(prefilterEnvMap);
        vec3 specularIrradiance = textureLod(prefilterEnvMap, R, roughness * specularTextureLevels).rgb;

        // Cook-Torrance specular split-sum approximation
        vec2 specularBRDF = texture(specularBRDF, vec2(cosLo, roughness)).rg;
        vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

        ambientLighting = diffuseIBL + specularIBL;
        ambientLighting *= material.environmentLightIntensity;
    }

	vec3 color = directLighting + ambientLighting;

	outColor = gamma_map(color, 2.2);
}