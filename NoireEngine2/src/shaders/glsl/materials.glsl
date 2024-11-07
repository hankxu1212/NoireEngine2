///////////////////////////////////////////////
// Material Buffers: aliased for float and ints
///////////////////////////////////////////////
layout(set=1, binding=4, scalar) readonly buffer MaterialBufferInt {
	int MATERIAL_BUFFER_INT[];
};

layout(set=1, binding=4, scalar) readonly buffer MaterialBufferFloat {
	float MATERIAL_BUFFER_F[];
};

///////////////////////////////////////////////
// Object buffers: passing in object information
///////////////////////////////////////////////
struct ObjectDesc
{
	uint offset;
	uint id;
};

layout(set=1, binding=5, std430) readonly buffer Objects {
	ObjectDesc OBJECTS[];
};

struct LambertianMaterial
{
	vec4 albedo;
	float environmentLightIntensity;
	float normalStrength;
	int albedoTexId;
	int normalTexId;
};

struct PBRMaterial
{
	vec4 albedo;
	float environmentLightIntensity;
	int albedoTexId;
	int normalTexId;
	int displacementTexId;
	float heightScale;
	int roughnessTexId;
	int metallicTexId;
	float roughness;
	float metallic;
	float normalStrength;
};

#define FLT_AT(i) MATERIAL_BUFFER_F[offset + i]
#define INT_AT(i) MATERIAL_BUFFER_INT[offset + i]

LambertianMaterial LAMBERTIAN_MATERIAL_ReadFrombuffer(uint offset)
{
    LambertianMaterial mat;
    
    mat.albedo = vec4(
        FLT_AT(0), 
        FLT_AT(1),
        FLT_AT(2),
        0
    );

    mat.environmentLightIntensity = FLT_AT(4);

    mat.normalStrength = FLT_AT(5);
	
    mat.albedoTexId = INT_AT(6);

    mat.normalTexId = INT_AT(7);

    return mat;
}

PBRMaterial PBR_MATERIAL_ReadFrombuffer(uint offset)
{
    PBRMaterial mat;

    mat.albedo = vec4(
        FLT_AT(0), 
        FLT_AT(1),
        FLT_AT(2),
        0
    );

    mat.environmentLightIntensity = FLT_AT(4);

    mat.albedoTexId = INT_AT(5);

    mat.normalTexId = INT_AT(6);

    mat.displacementTexId = INT_AT(7);
    
    mat.heightScale = FLT_AT(8);

    mat.roughnessTexId = INT_AT(9);

    mat.metallicTexId = INT_AT(10);

    mat.roughness = FLT_AT(11);

    mat.metallic = FLT_AT(12);

    mat.normalStrength = FLT_AT(13);

    return mat;
}
