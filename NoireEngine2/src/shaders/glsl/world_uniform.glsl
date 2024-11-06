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