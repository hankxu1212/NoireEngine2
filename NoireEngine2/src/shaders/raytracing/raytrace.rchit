#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rtxcore.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(buffer_reference, scalar) buffer Vertices 
{ 
    Vertex v[]; 
}; // Positions of an object

layout(buffer_reference, scalar) buffer Indices 
{ 
    ivec3 i[]; 
}; // Triangle indices

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

struct ObjDesc
{
    int      txtOffset;             // Texture index offset in the array of textures
    Vertices vertexAddress;         // Address of the Vertex buffer
    Indices indexAddress;          // Address of the index buffer
};

layout(set = 0, binding = 2, scalar) buffer ObjDesc_ 
{ 
    ObjDesc i[]; 
} objDesc;

void main()
{
    // Object data
    ObjDesc objResource = objDesc.i[gl_InstanceCustomIndexEXT];
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

    // Vector toward the light
    vec3  L;
    float lightIntensity = 100;
    float lightDistance  = 100000.0;
    vec3 lightPosition = vec3(2,2,2);

    // point light for now
    vec3 lDir      = lightPosition - worldPos;
    lightDistance  = length(lDir);
    lightIntensity = lightIntensity / (lightDistance * lightDistance);
    L              = normalize(lDir);

    // Lambertian
    vec3 matDiffuse = vec3(0.5, 0.5, 0.2);
    float dotNL = max(dot(worldNrm, L), 0.0);
    vec3 diffuse = matDiffuse * dotNL;

    vec3  specular    = vec3(0);
    float attenuation = 1;

    prd.hitValue = vec3(lightIntensity * attenuation * (diffuse + specular));
}
