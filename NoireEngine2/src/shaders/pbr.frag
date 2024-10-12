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

	// Outgoing light direction (vector from world-space fragment position to the "eye").
	vec3 Lo = normalize(vec3(scene.cameraPos) - LIGHTING_VAR_TANGENT_FRAG_POS);
	
	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0, dot(n, Lo));
		
	// Specular reflection vector.
	vec3 Lr = 2.0 * cosLo * n - Lo;
	
	float metalness = material.metallic;
	float roughness = material.roughness;

	// Fresnel reflectance at normal incidence (for metals use albedo color).
	vec3 F0 = mix(Fdielectric, albedo, metalness);

	vec3 directLighting = vec3(0);
	for (int i = 0; i < min(MAX_LIGHTS_PER_OBJ, scene.numLights); ++i)
	{
		vec3 radiance = CalcLight(i, CURR_LIGHT.type);
	
		// Direct lighting calculation for analytical lights.
		vec3 Li = normalize(vec3(CURR_LIGHT.position) - LIGHTING_VAR_TANGENT_FRAG_POS);
	
		// Half-vector between Li and Lo.
		vec3 Lh = normalize(Li + Lo);
	
		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(n, Li));
		float cosLh = max(0.0, dot(n, Lh));
	
		// Calculate Fresnel term for direct lighting. 
		vec3 F  = FresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
		// Calculate normal distribution for specular BRDF.
		float D = DistributionGGX(cosLh, roughness);
		// Calculate geometric attenuation for specular BRDF.
		float G = GeometrySmith(cosLi, cosLo, roughness);
	
		// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
		// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
		// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
		vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);
	
		vec3 diffuseBRDF = kd * albedo;
	
		// Cook-Torrance specular microfacet BRDF.
		vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
	
		// Total contribution for this light.
		directLighting += (diffuseBRDF + specularBRDF) * radiance * cosLi;
	}


	// IDL
	vec3 ambientLighting;
	{
		vec3 environmentalLight = vec3(texture(lambertianIDL, n));
		ambientLighting = environmentalLight * vec3(material.albedo);
		ambientLighting *= material.environmentLightIntensity;
	}

	vec3 color = directLighting + ambientLighting;

	outColor = gamma_map(color, 2.2);
}