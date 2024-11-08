///////////////////////////////////////////////
// Set 0: world
///////////////////////////////////////////////
layout(set=0,binding=0,std140) uniform World {
	mat4 view;
	mat4 viewProj;
	mat4 viewInverse;
	mat4 projInverse;
	vec4 cameraPos;
	uvec4 numLights;
	int pcfSamples;
	int occluderSamples;
}scene;

///////////////////////////////////////////////
// Set 1.0: transforms
///////////////////////////////////////////////
struct Transform 
{
	mat4 localToClip;
	mat4 model;
	mat4 modelNormal;
};

layout(set=1, binding=0, std140) readonly buffer Transforms {
	Transform TRANSFORMS[];
};

///////////////////////////////////////////////
// Set 1.1: directional lights
///////////////////////////////////////////////
struct DirLight
{
	mat4 lightspaces[4];
	vec4 color;
	vec4 direction;
	vec4 splitDepths;
	float angle;
	float intensity;
	int shadowOffset;
	float shadowStrength;
};

layout(set=1, binding=1, std140) readonly buffer DirLights {
	DirLight DIR_LIGHTS[];
};

///////////////////////////////////////////////
// Set 1.2: point lights
///////////////////////////////////////////////
struct PointLight
{
	mat4 lightspaces[6];
	vec4 color;
	vec4 position;
	float intensity;
	float radius;
	float limit;
	int shadowOffset;
	float shadowStrength;

	float pad;
	float pad2;
	float pad3;
};

layout(set=1, binding=2, std140) readonly buffer PointLights {
	PointLight POINT_LIGHTS[];
};

///////////////////////////////////////////////
// Set 1.3: Spotlights
///////////////////////////////////////////////
struct SpotLight
{
	mat4 lightspace;
	vec4 color;
	vec4 position;
	vec4 direction;
	float intensity;
	float radius;
	float limit;
	float fov;
	float blend;
	int shadowOffset;
	float shadowStrength;
	float nearClip;
};

layout(set=1, binding=3, std140) readonly buffer SpotLights {
	SpotLight SPOT_LIGHTS[];
};

///////////////////////////////////////////////
// Set 1.4: Material Buffers: aliased for float and ints
///////////////////////////////////////////////
layout(set=1, binding=4, scalar) readonly buffer MaterialBufferInt {
	int MATERIAL_BUFFER_INT[];
};

layout(set=1, binding=4, scalar) readonly buffer MaterialBufferFloat {
	float MATERIAL_BUFFER_F[];
};

///////////////////////////////////////////////
// Set 1.5: Object buffers: passing in object information
// including vertex and index buffer addresses
///////////////////////////////////////////////
struct Vertex
{
	vec3 pos;
	vec3 nrm;
	vec4 tangent;
	vec2 texCoord;
};

layout(buffer_reference, scalar) buffer Vertices 
{ 
    Vertex v[]; 
}; // Positions of an object

layout(buffer_reference, scalar) buffer Indices 
{ 
    ivec3 i[]; 
}; // Triangle indices

struct ObjectDesc
{
    Vertices vertexAddress;         // Address of the Vertex buffer
    Indices indexAddress;          // Address of the index buffer
	uint offset;
	uint materialType;
};

layout(set=1, binding=5, scalar) readonly buffer Objects {
	ObjectDesc OBJECTS[];
};

///////////////////////////////////////////////
// Set 2: Descriptor indexed textures
///////////////////////////////////////////////
layout (set = 2, binding = 0) uniform sampler2D textures[];

///////////////////////////////////////////////
// Set 3: IDL, skybox, and environment
///////////////////////////////////////////////
layout (set = 3, binding = 0) uniform samplerCube skybox;
layout (set = 3, binding = 1) uniform samplerCube diffuseIrradiance;
layout (set = 3, binding = 2) uniform sampler2D specularBRDF;
layout (set = 3, binding = 3) uniform samplerCube prefilterEnvMap;

///////////////////////////////////////////////
// Set 4: Descriptor indexed shadowmaps
///////////////////////////////////////////////
layout (set = 4, binding = 0) uniform sampler2D shadowMaps[];