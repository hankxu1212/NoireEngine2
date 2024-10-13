// UE4 inspired Lambetrtian diffuse BRDF + Cook-Torrance microfacet specular BRDF + IBL

#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec4 inTangent;

layout(location=0) out vec4 outColor;

#include "glsl/world_uniform.glsl"
#include "glsl/utils.glsl"

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);
#include "glsl/pbr.glsl"

vec3 n; // the lighting.glsl depends on this
#include "glsl/lighting.glsl"
#include "glsl/cubemap.glsl"

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
    int displacementTexId;
    float heightScale;
	int roughnessTexId;
	int metallicTexId;
	float roughness;
	float metallic;
	float normalStrength;
	float environmentLightIntensity;
} material;

// parallax mappings
const int parallax_numLayers = 32;
const float parallax_layerDepth = 1.0 / float(parallax_numLayers);
#include "glsl/parallax.glsl"

void main() {
    // calculate TBN
    n = normalize(inNormal);
    vec3 tangent = inTangent.xyz;
    tangent = normalize(tangent - dot(tangent, n) * n);
    vec3 bitangent = normalize(cross(n, inTangent.xyz) * inTangent.w);
    mat3 TBN = mat3(tangent, bitangent, n);

    // view direction
    vec3 V = normalize(vec3(scene.cameraPos) - inPosition);

    // uv
    vec2 UV = inTexCoord;
    float height = 0;

    // parallax occlusion mapping
    mat3 invTBN = transpose(TBN);
    vec3 tangentV = invTBN * V;
    if (material.displacementTexId >= 0 && material.heightScale > 0){
        // can use contact refinement parallax (ParallaxOcclusionMapping2), kinda broken tho
        vec3 uvh = ParallaxOcclusionMapping(inTexCoord, tangentV);
        UV = uvh.xy;
        height = uvh.z;
    }

	// normal mapping
    // Perform sampling before (potentially) discarding.
	// This is to avoid implicit derivatives in non-uniform control flow.
	if (material.normalTexId >= 0)
	{
        vec3 localNormal = 2.0 * textureLod(textures[material.normalTexId], UV, 0.0).rgb - 1.0;
        n = TBN * localNormal;
		n.xy *= material.normalStrength;
		n = normalize(n);
	}

	// albedo 
	vec3 albedo = vec3(material.albedo);
	if (material.albedoTexId >= 0)
		albedo *= texture(textures[material.albedoTexId], UV).rgb;

    if(UV.x > 1.0 || UV.y > 1.0 || UV.x < 0.0 || UV.y < 0.0)
        discard;

    // roughness
    float roughness = material.roughness;
    if (material.roughnessTexId >= 0)
        roughness = textureLod(textures[material.roughnessTexId], UV, 0.0).a;

    // metallic
    float metalness = material.metallic;
    if (material.metallicTexId >= 0)
        metalness = textureLod(textures[material.metallicTexId], UV, 0.0).a;


	// light out
    float cosLo = max(0.0, dot(n, V));

    // specular reflection
    vec3 R = reflect(-V, n);

    // Fresnel
    vec3 F0 = mix(Fdielectric, albedo, metalness);

    vec3 directLighting = vec3(0);
    for (int i = 0; i < min(MAX_LIGHTS_PER_OBJ, scene.numLights); ++i)
    {
        vec3 radiance = CalcLight(i, CURR_LIGHT.type);

        // Incident light direction.
        vec3 Li = normalize(vec3(CURR_LIGHT.position) - inPosition);
        vec3 Lh = normalize(Li + V);

        // Cosines of angles between normal and light vectors.
        float cosLi = max(0.0, dot(n, Li));
        float cosLh = max(0.0, dot(n, Lh));

        vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, V)), roughness);
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

        vec3 F = FresnelSchlickRoughness(F0, cosLo, roughness);

        // Diffuse contribution for ambient light
        vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);
        vec3 diffuseIBL = kd * albedo * irradiance;

        // Specular IBL from pre-filtered environment map
        const float MAX_REFLECTION_LOD = 6.0; // todo: param/const
	    float lod = roughness * MAX_REFLECTION_LOD;
	    float lodf = floor(lod);
	    float lodc = ceil(lod);
	    vec3 a = textureLod(prefilterEnvMap, R, lodf).rgb;
	    vec3 b = textureLod(prefilterEnvMap, R, lodc).rgb;
        vec3 specularIrradiance = mix(a, b, lod - lodf);

        // Cook-Torrance specular split-sum approximation
        vec2 specularBRDF = texture(specularBRDF, vec2(cosLo, roughness)).rg;
        vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

        ambientLighting = diffuseIBL + specularIBL * material.environmentLightIntensity;
    }


	vec3 color = directLighting + ambientLighting;
    color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));

	outColor = gamma_map(color, 2.2);
}