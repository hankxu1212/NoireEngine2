#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_ARB_gpu_shader_int64 : enable

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec4 inTangent;
layout(location=4) in vec3 inViewPos;
layout(location=5) flat in int instanceID;

layout(location=0) out vec4 outColor;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outEmission;

#include "host.glsl"

#include "glsl/utils.glsl"

vec3 n;
#include "glsl/lighting.glsl"

#include "glsl/materials.glsl"

void main() 
{
	LambertianMaterial material = LAMBERTIAN_MATERIAL_ReadFrombuffer(OBJECTS[instanceID].offset);

    // calculate TBN
    n = normalize(inNormal);
    vec3 tangent = inTangent.xyz;
    tangent = normalize(tangent - dot(tangent, n) * n);
    vec3 bitangent = normalize(cross(n, inTangent.xyz) * inTangent.w);
    mat3 TBN = mat3(tangent, bitangent, n);

	// normal mapping
	if (material.normalTexId >= 0)
	{
        n = 2.0 * texture(textures[material.normalTexId], inTexCoord).rgb - 1.0;
		n.xy *= material.normalStrength;
		n = normalize(n);
        n = normalize(TBN * n);
	}

	// sample diffuse irradiance
	vec3 ambientLighting = texture(diffuseIrradiance, n).rgb * material.environmentLightIntensity;

	// obtain reflection color from rtx
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
    vec3 reflectionColor = imageLoad(raytracedReflections, pixelCoord).rgb;

	// material
	vec3 texColor = material.albedo.rgb;
	if (material.albedoTexId >= 0)
		texColor *= texture(textures[material.albedoTexId], inTexCoord).rgb;

	// direct lighting and shadows
	vec3 directLighting = DirectLighting();

	vec3 color = texColor * (directLighting + ambientLighting + reflectionColor * 0.5); // just an arbitrary multiplier
	
	outColor = vec4(color, 1);
	outNormal = vec4(inPosition, uintBitsToFloat(CompressUnitVec(n)));

	// check whether fragment output is higher than threshold, if so output as brightness color
    outEmission = vec4(0.0, 0.0, 0.0, 1.0);
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
	    outEmission = outColor;

	// handle mouse picking
    // get the depth and scale it up by
    // the total number of buckets in depth array
    uint zIndex = uint(gl_FragCoord.z * DEPTH_ARRAY_SCALE);
    if( length( scene.mousePosition - gl_FragCoord.xy) < 1)
        mousePicking.data[zIndex] = OBJECTS[instanceID].entityID;
}