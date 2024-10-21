layout(set=0,binding=0,std140) uniform World {
	vec4 cameraPos;
	
	// shadow stuff temporary
	mat4 lightSpace;
	vec4 lightPos;
	// Used for depth map visualization
	float zNear;
	float zFar;

	int numDirLights;
	int numPointLights;
	int numSpotLights;
}scene;