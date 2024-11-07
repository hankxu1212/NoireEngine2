vec3 ParallaxOcclusionMapping(vec2 uv, vec3 V, float heightScale, int displacementTexId)
{
    float viewCorrection = (-V.z) + 2.0;

	float currLayerDepth = 0.0;
	vec2 dUV = V.xy * heightScale / (viewCorrection * parallax_numLayers);
	float height = 1.0 - textureLod(textures[displacementTexId], uv, 0.0).r;
	for (int i = 0; i <parallax_numLayers; i++) {
		currLayerDepth += parallax_layerDepth;
		uv -= dUV;
		height = 1.0 - textureLod(textures[displacementTexId], uv, 0.0).r;
		if (height < currLayerDepth) {
			break;
		}
	}
	vec2 prevUV = uv + dUV;
	float nextDepth = height - currLayerDepth;
	float prevDepth = 1.0 - textureLod(textures[displacementTexId], prevUV, 0.0).r - currLayerDepth + parallax_layerDepth;
	return vec3(mix(uv, prevUV, nextDepth / (nextDepth - prevDepth)), height);
}