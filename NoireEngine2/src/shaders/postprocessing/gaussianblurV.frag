#version 450

layout (set=0, binding=4) uniform sampler2D samplerColor;

layout(push_constant) uniform BlurInfo {
    float blurScale;
    float blurStrength;
};

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() 
{
	vec2 tex_offset = 1.0 / textureSize(samplerColor, 0) * blurScale; // gets size of single texel
	vec3 result = texture(samplerColor, inUV).rgb * weight[0]; // current fragment's contribution
	for(int i = 1; i < 5; ++i)
	{
		// V
		result += texture(samplerColor, inUV + vec2(0.0, tex_offset.y * i)).rgb * weight[i] * blurStrength;
		result += texture(samplerColor, inUV - vec2(0.0, tex_offset.y * i)).rgb * weight[i] * blurStrength;
	}
	outFragColor = vec4(result, 1.0);
}