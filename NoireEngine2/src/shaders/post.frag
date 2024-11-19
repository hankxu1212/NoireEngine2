#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1, rgba8) uniform image2D G_Color;
layout(set = 0, binding = 3) uniform sampler2D G_Emission;
layout(set = 1, binding = 2, r32f) uniform image2D raytracedAOSampler;
layout(set = 1, binding = 3, rgba8) uniform image2D raytracedTransparencySampler;
layout(set = 1, binding = 4, r8) uniform image2D transparencyMask;

#include "glsl/utils.glsl"

layout(push_constant) uniform params_
{
    int useToneMapping;
	int useBloom;
	float exposure;
	int useAO;
};

void main()
{
    ivec2 texelCoord = ivec2(gl_FragCoord.xy);
	
	vec3 color = imageLoad(G_Color, texelCoord).rgb;

	// ambient occlusion
	float ao = 0;
	if (useAO == 1)
		ao = imageLoad(raytracedAOSampler, texelCoord).x;
	color *= (ao == 0 ? 1 : ao);

	// transparency
	vec3 transparentColor = imageLoad(raytracedTransparencySampler, texelCoord).rgb;
	float mask = imageLoad(transparencyMask, texelCoord).r;
	
	color = mix(color, transparentColor, mask);

	// todo: bloom is incorrect atm cuz it applies over the transparency mask
	// bloom
	if (useBloom == 1)
	{
		vec3 bloomColor = texture(G_Emission, uv).rgb;  
	    
		// tone mapping for bloom
		bloomColor = vec3(1.0) - exp(-bloomColor * exposure);
	
		color += bloomColor;
	}
	
	// tone map
	if (useToneMapping == 1)
		color = ACES(color);
	
	fragColor = vec4(color, 1);
}
