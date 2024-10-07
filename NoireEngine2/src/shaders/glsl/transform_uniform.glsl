struct Transform {
	mat4 localToClip;
	mat4 model;
	mat4 modelNormal;
};

layout(set=1, binding=0, std140) readonly buffer Transforms {
	Transform TRANSFORMS[];
};
