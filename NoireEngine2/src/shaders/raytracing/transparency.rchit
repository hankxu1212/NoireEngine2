#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "reflections.glsl"
#include "../host.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(set = 5, binding = 0) uniform accelerationStructureEXT topLevelAS;

#include "../glsl/materials.glsl"

#define MAX_REFLECTION_LOD 6
const vec3 Fdielectric = vec3(0.04);
#include "../glsl/pbr.glsl"

layout(constant_id = 0) const int MATERIAL_TYPE = 0;

void main()
{
    // Object data
    ObjectDesc objResource = OBJECTS[gl_InstanceCustomIndexEXT];
    Indices indices = Indices(objResource.indexAddress);
    Vertices vertices = Vertices(objResource.vertexAddress);

    // Indices of the triangle
    ivec3 ind = indices.i[gl_PrimitiveID];

    // Vertex of the triangle
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // Computing the coordinates of the hit position
    const vec3 pos      = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space

    // Computing the normal at hit position
    const vec3 nrm      = v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z;
    const vec3 n = normalize(vec3(nrm * gl_WorldToObjectEXT));  // Transforming the normal to world space

    const vec2 UV = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

    // return early if is first bounce:
    if (prd.depth == 0)
    {
        prd.done      = 0;
        prd.rayOrigin = worldPos;
        prd.rayDir    = reflect(gl_WorldRayDirectionEXT, n);
        prd.hitValue = vec3(0);
        return;
    }

    vec3 color;
    if (MATERIAL_TYPE == 0) // lambertian
    {
        LambertianMaterial material = LAMBERTIAN_MATERIAL_ReadFrombuffer(objResource.offset);

        // sample diffuse irradiance
	    vec3 ambientLighting = texture(diffuseIrradiance, n).rgb * material.environmentLightIntensity;

	    // material
	    vec3 texColor = material.albedo.rgb;
	    if (material.albedoTexId >= 0)
		    texColor *= texture(textures[material.albedoTexId], UV).rgb;

	    color = texColor * ambientLighting;
    }
    else if (MATERIAL_TYPE == 1) // pbr
    {
    	PBRMaterial material = PBR_MATERIAL_ReadFrombuffer(objResource.offset);
        vec3 V = normalize(vec3(scene.cameraPos) - worldPos);

        // albedo 
	    vec3 albedo = material.albedo.rgb;
	    if (material.albedoTexId >= 0)
		    albedo *= texture(textures[material.albedoTexId], UV).rgb;

        // roughness
        float roughness = material.roughness;
        if (material.roughnessTexId >= 0)
            roughness *= texture(textures[material.roughnessTexId], UV).a;

        // metallic
        float metalness = material.metallic;
        if (material.metallicTexId >= 0)
            metalness *= texture(textures[material.metallicTexId], UV).a;

       	// light out
        float cosLo = max(0.0, dot(n, V));

        // specular reflection
        vec3 R = reflect(-V, n);

        // Fresnel
        vec3 F0 = mix(Fdielectric, albedo, metalness);

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

        color = diffuseIBL + specularIBL * material.environmentLightIntensity;

        // multiply attenuation with specular
        prd.attenuation *= F;
    }

    // Reflection
    prd.done      = MATERIAL_TYPE == 0 ? 1 : 0; // done if hit a lambertian surface
    prd.rayOrigin = worldPos;
    prd.rayDir    = reflect(gl_WorldRayDirectionEXT, n);
    prd.hitValue = color;
}
