layout(set=0,binding=0,std140) uniform World {
	vec4 cameraPos;
	int numDirLights;
	int numPointLights;
	int numSpotLights;
}scene;