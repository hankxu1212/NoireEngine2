struct hitPayload
{
  vec3 hitValue;
};

layout(set=1,binding=0,std140) uniform World {
	mat4 view;
	mat4 viewProj;
	mat4 viewInverse;
	mat4 projInverse;
	vec4 cameraPos;
	uvec4 numLights;
	int pcfSamples;
	int occluderSamples;
}scene;

layout(push_constant) uniform constants 
{ 
    vec4  clearColor;
	vec3  lightPosition;
	float lightIntensity;
	int   lightType;
} rayConstants;

struct Vertex
{
	vec3 pos;
	vec3 nrm;
	vec4 tangent;
	vec2 texCoord;
};