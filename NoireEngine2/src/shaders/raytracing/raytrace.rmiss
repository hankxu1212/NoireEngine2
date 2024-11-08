#version 460
#extension GL_EXT_ray_tracing : require

#include "rtxcore.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
    prd.hitValue = rayConstants.clearColor.xyz * 0.8 * prd.attenuation;
}