#define MAX_NUM_TOTAL_LIGHTS 20
#define MAX_LIGHTS_PER_OBJ 8

struct Light {
    vec4 color;
    vec4 position;
    vec4 direction;
	float radius;
	float limit;
	float intensity;
	float fov;
	float blend;
	int type;
};

layout(set=0,binding=0,std140) uniform World {
	Light lights[MAX_NUM_TOTAL_LIGHTS];
	int numLights;
	vec4 cameraPos;
	
	// shadow stuff temporary
	mat4 lightSpace;
	vec4 lightPos;
	// Used for depth map visualization
	float zNear;
	float zFar;
}scene;