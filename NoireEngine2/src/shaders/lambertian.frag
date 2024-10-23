#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec4 inTangent;

layout(location=0) out vec4 outColor;

#include "glsl/world_uniform.glsl"
#include "glsl/utils.glsl"

vec3 n;
#include "glsl/lighting.glsl"

layout (set = 2, binding = 0) uniform sampler2D textures[]; // global texture array, indexed
layout (set = 3, binding = 1) uniform samplerCube lambertianIDL;
layout (set = 4, binding = 0) uniform sampler2D shadowMaps[]; // shadow maps, indexed

layout( push_constant ) uniform constants
{
	vec4 albedo;
	int albedoTexId;
	int normalTexId;
	float normalStrength;
	float environmentLightIntensity;
} material;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float shadowBias;
#include "glsl/shadows.glsl"

void main() {
    // calculate TBN
    n = normalize(inNormal);
    vec3 tangent = inTangent.xyz;
    tangent = normalize(tangent - dot(tangent, n) * n);
    vec3 bitangent = normalize(cross(n, inTangent.xyz) * inTangent.w);
    mat3 TBN = mat3(tangent, bitangent, n);

	// normal mapping
	if (material.normalTexId >= 0)
	{
        vec3 localNormal = 2.0 * textureLod(textures[material.normalTexId], inTexCoord, 0.0).rgb - 1.0;
        n = TBN * localNormal;
		n.xy *= material.normalStrength;
		n = normalize(n);
	}

	vec3 lightsSum = DirectLighting();

	// sample diffuse irradiance
	vec3 ambientLighting = texture(lambertianIDL, n).rgb * material.environmentLightIntensity;

	// material
	vec3 texColor = material.albedo.rgb;
	if (material.albedoTexId >= 0)
		texColor *= texture(textures[material.albedoTexId], inTexCoord).rgb;

	// shadow calculation
	float shadow = 1;
	for (int i = 0; i < scene.numSpotLights; ++i)
	{
		shadowBias = max(0.05 * (1.0 - dot(n, vec3(SPOT_LIGHTS[i].position) - inPosition)), 0.005);  
		vec4 shadowCoord = biasMat * SPOT_LIGHTS[i].lightspace * vec4(inPosition, 1.0);
		
		// perform perspective divide
		shadowCoord /= shadowCoord.w;

		// when z/w goes outside the normalized range [-1, 1], they lie outside the view frustum
		// similarly for x,y, they need to be in [0, 1]
		if (abs(shadowCoord.z) <= 1
			&& shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0
			&& shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0)
		{
			shadow *= PCSS(shadowCoord.xy, shadowCoord.z, shadowBias, i);
		}
	}

	vec3 color = vec3(material.albedo) * texColor * (lightsSum + ambientLighting);
	color *= shadow;
	
	color = ACES(color);
	outColor = gamma_map(color, 2.2f);
}