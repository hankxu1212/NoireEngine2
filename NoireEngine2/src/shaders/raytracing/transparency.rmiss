#version 460
#extension GL_EXT_ray_tracing : require

#include "reflections.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout (set = 3, binding = 0) uniform samplerCube skybox;

void main()
{
    prd.hitValue = texture(skybox, prd.rayDir).rgb;
    prd.done = 1;
}