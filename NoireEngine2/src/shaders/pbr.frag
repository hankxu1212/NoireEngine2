// UE4 inspired Lambetrtian diffuse BRDF + Cook-Torrance microfacet specular BRDF + IBL

#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

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

mat3 GetTangents()
{
    vec3 Q1  = dFdx(inPosition);
    vec3 Q2  = dFdy(inPosition);
    vec2 st1 = dFdx(inTexCoord);
    vec2 st2 = dFdy(inTexCoord);

    vec3 N  = normalize(inNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    return mat3(T, B, N);
}

// parallax mappings
vec2 UV; // uv after parallax mapping
int numLayers = 16;
#include "glsl/parallax.glsl"

void main() {
    mat3 TBN = GetTangents();
    vec3 V = normalize(vec3(scene.cameraPos) - inPosition);

    // parallax occlusion mapping
    UV = inTexCoord;
    // if (material.displacementTexId >= 0 && material.heightScale > 0){
        // UV = ParallaxOcclusionMapping(inTexCoord, V);
    // }


	// normal mapping
    n = normalize(inNormal);
	if (material.normalTexId >= 0)
	{
        vec3 tangentNormal = textureLod(textures[material.normalTexId], UV, 0.0).rgb * 2.0 - 1.0;
		n = TBN * tangentNormal;
		n.xy *= material.normalStrength;
		n = normalize(n);
	}

	// albedo 
	vec3 albedo = vec3(1);
	if (material.albedoTexId >= 0)
		albedo = texture(textures[material.albedoTexId], UV).rgb;
	albedo *= vec3(material.albedo);

    if(UV.x > 1.0 || UV.y > 1.0 || UV.x < 0.0 || UV.y < 0.0)
        discard;

    float metalness = material.metallic;
    float roughness = material.roughness;

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