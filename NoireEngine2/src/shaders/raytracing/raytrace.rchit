#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rtxcore.glsl"
#include "../host.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(set = 5, binding = 0) uniform accelerationStructureEXT topLevelAS;

#include "../glsl/materials.glsl"

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
    const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  // Transforming the normal to world space

    uint matWorkflow = objResource.materialType;
    vec3 color;
    if (matWorkflow == 0) // lambertian
    {
        LambertianMaterial material = LAMBERTIAN_MATERIAL_ReadFrombuffer(objResource.offset);
        color = material.albedo.rgb;
    }
    else if (matWorkflow == 1) // pbr
    {
    	PBRMaterial material = PBR_MATERIAL_ReadFrombuffer(objResource.offset);
        color = material.albedo.rgb;
    }

    // Reflection
    vec3 origin = worldPos;
    vec3 rayDir = reflect(gl_WorldRayDirectionEXT, worldNrm);
    prd.attenuation *= 1;
    prd.done      = 0;
    prd.rayOrigin = origin;
    prd.rayDir    = rayDir;

    prd.hitValue = color * 0.3;
}
