vec2 ParallaxOcclusionMapping(vec2 uv, vec3 viewDir) 
{
	float layerDepth = 1.0 /numLayers;
	float currLayerDepth = 0.0;
	vec2 deltaUV = viewDir.xy * material.heightScale / (viewDir.z *numLayers);
	vec2 currUV = uv;
	float height = 1.0 - textureLod(textures[material.displacementTexId], currUV, 0.0).r;
	for (int i = 0; i <numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureLod(textures[material.displacementTexId], currUV, 0.0).r;
		if (height < currLayerDepth) {
			break;
		}
	}
	vec2 prevUV = currUV + deltaUV;
	float nextDepth = height - currLayerDepth;
	float prevDepth = 1.0 - textureLod(textures[material.displacementTexId], prevUV, 0.0).r - currLayerDepth + layerDepth;
	return mix(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}