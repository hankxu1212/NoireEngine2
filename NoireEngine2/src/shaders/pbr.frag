// UE4 inspired Lambetrtian diffuse BRDF + Cook-Torrance microfacet specular BRDF + IBL

#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec4 inTangent;
layout(location=4) in vec3 inViewPos;
layout(location=5) flat in uint instanceID;

layout(location=0) out vec4 outColor;

#include "host.glsl"

#include "glsl/utils.glsl"

// Constant normal incidence Fresnel factor for all dielectrics.
#define MAX_REFLECTION_LOD 6
#define GGX_MIP_LEVELS 6
const vec3 Fdielectric = vec3(0.04);
#include "glsl/pbr.glsl"

// global variables so no need to pass ton of variables
vec3 n, F0, V, albedo;
float roughness, metalness, cosLo; 
vec3 CalcPBRDirectLighting(vec3 radiance, vec3 Li);
vec3 DirectLightingPBR();
#include "glsl/lighting.glsl"

#include "glsl/materials.glsl"

// parallax mappings
const int parallax_numLayers = 32;
const float parallax_layerDepth = 1.0 / float(parallax_numLayers);
#include "glsl/parallax.glsl"

void main() 
{
	PBRMaterial material = PBR_MATERIAL_ReadFrombuffer(OBJECTS[instanceID].offset);

    // calculate TBN
    n = normalize(inNormal);
    vec3 tangent = inTangent.xyz;
    tangent = normalize(tangent - dot(tangent, n) * n);
    vec3 bitangent = normalize(cross(n, inTangent.xyz) * inTangent.w);
    mat3 TBN = mat3(tangent, bitangent, n);

    // view direction
    V = normalize(vec3(scene.cameraPos) - inPosition);

    // uv
    vec2 UV = inTexCoord;
    float height = 0;

    // parallax occlusion mapping
    mat3 invTBN = transpose(TBN);
    vec3 tangentV = invTBN * V;
    if (material.displacementTexId >= 0 && material.heightScale > 0){
        // can use contact refinement parallax (ParallaxOcclusionMapping2), kinda broken tho
        vec3 uvh = ParallaxOcclusionMapping(inTexCoord, tangentV, material.heightScale, material.displacementTexId);
        UV = uvh.xy;
        height = uvh.z;
    }

	// normal mapping
	if (material.normalTexId >= 0)
	{
        n = 2.0 * texture(textures[material.normalTexId], UV).rgb - 1.0;
		n.xy *= material.normalStrength;
		n = normalize(n);
        n = normalize(TBN * n);
	}

	// albedo 
	albedo = material.albedo.rgb;
	if (material.albedoTexId >= 0)
		albedo *= texture(textures[material.albedoTexId], UV).rgb;

    // roughness
    roughness = material.roughness;
    if (material.roughnessTexId >= 0)
        roughness *= texture(textures[material.roughnessTexId], UV).a;

    // metallic
    metalness = material.metallic;
    if (material.metallicTexId >= 0)
        metalness *= texture(textures[material.metallicTexId], UV).a;

	// light out
    cosLo = max(0.0, dot(n, V));

    // specular reflection
    vec3 R = reflect(-V, n);

    // Fresnel
    F0 = mix(Fdielectric, albedo, metalness);

    // direct analytical lighting
    vec3 directLighting = DirectLightingPBR();

    // IBL: ambient lighting
    vec3 ambientLighting;
    {
        // lambertian diffuse irradiance
        vec3 irradiance = texture(diffuseIrradiance, n).rgb / PI;

        vec3 F = FresnelSchlickRoughness(F0, cosLo, roughness);

        // Diffuse contribution for ambient light
        vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);
        vec3 diffuseIBL = kd * albedo * irradiance;

        // Specular IBL from pre-filtered environment map
	    float lod = roughness * MAX_REFLECTION_LOD;
        vec3 specularIrradiance = textureLod(prefilterEnvMap, R, lod).rgb;

        // Cook-Torrance specular split-sum approximation
        vec2 specularBRDF = texture(specularBRDF, vec2(cosLo, roughness)).rg;
        vec3 specularIBL = (F * specularBRDF.x + specularBRDF.y) * specularIrradiance;

        ambientLighting = diffuseIBL + specularIBL * material.environmentLightIntensity;
    }

	vec3 color = directLighting + ambientLighting;

    // color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    color = ACES(color);
	outColor = vec4(color, 1);
}

vec3 CalcPBRDirectLighting(vec3 radiance, vec3 Li)
{
	// Incident light direction.
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
    
    return (diffuseBRDF + specularBRDF) * radiance * cosLi;
}

vec3 DirectLightingPBR()
{
	vec3 lightsSum = vec3(0);

    int shadowMapOffsetId = 0;

	for (int i = 0; i < scene.numLights[0]; ++i)
	{
        float radiance = DirLightRadiance(i);

		if (radiance > 0 && DIR_LIGHTS[i].shadowOffset > 0)
			radiance *= DirLightShadow(i, shadowMapOffsetId);

		shadowMapOffsetId += DIR_LIGHTS[i].shadowOffset;
		lightsSum += CalcPBRDirectLighting(radiance * vec3(DIR_LIGHTS[i].color), -vec3(DIR_LIGHTS[i].direction));
	}

	for (int i = 0; i < scene.numLights[1]; ++i)
	{
    	float radiance = PointLightRadiance(i);
		
		if (radiance > 0 && POINT_LIGHTS[i].shadowOffset > 0)
			radiance *= PointLightShadow(i, shadowMapOffsetId);
		
		shadowMapOffsetId += POINT_LIGHTS[i].shadowOffset;
		vec3 Li = normalize(vec3(POINT_LIGHTS[i].position) - inPosition);
		lightsSum += CalcPBRDirectLighting(radiance * vec3(POINT_LIGHTS[i].color), Li);
	}

	for (int i = 0; i < scene.numLights[2]; ++i)
	{
    	float radiance = SpotLightRadiance(i);

		if (radiance > 0 && SPOT_LIGHTS[i].shadowOffset > 0)
			radiance *= SpotLightShadow(i, shadowMapOffsetId);

		shadowMapOffsetId += SPOT_LIGHTS[i].shadowOffset;
		vec3 Li = normalize(vec3(SPOT_LIGHTS[i].position) - inPosition);
		lightsSum += CalcPBRDirectLighting(radiance * vec3(SPOT_LIGHTS[i].color), Li);
	}

	return lightsSum;
}
