vec3 ParallaxOcclusionMapping(vec2 uv, vec3 viewDir)
{
    float viewCorrection = (-viewDir.z) + 2.0;

	float currLayerDepth = 0.0;
	vec2 deltaUV = viewDir.xy * material.heightScale / (viewCorrection * parallax_numLayers);
	vec2 currUV = uv;
	float height = 1.0 - textureLod(textures[material.displacementTexId], currUV, 0.0).r;
	for (int i = 0; i <parallax_numLayers; i++) {
		currLayerDepth += parallax_layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureLod(textures[material.displacementTexId], currUV, 0.0).r;
		if (height < currLayerDepth) {
			break;
		}
	}
	vec2 prevUV = currUV + deltaUV;
	float nextDepth = height - currLayerDepth;
	float prevDepth = 1.0 - textureLod(textures[material.displacementTexId], prevUV, 0.0).r - currLayerDepth + parallax_layerDepth;
	return vec3(mix(currUV, prevUV, nextDepth / (nextDepth - prevDepth)), height);
}

// Contact-refinement parallax 
// TODO: fix the inverted height issue in this func
// https://www.artstation.com/blogs/andreariccardi/3VPo/a-new-approach-for-parallax-mapping-presenting-the-contact-refinement-parallax-mapping-technique
vec3 ParallaxOcclusionMapping2(vec2 uv, vec3 viewDir)
{
    float maxHeight = material.heightScale; //0.05;
    float minHeight = maxHeight*0.5;

    int numSteps = parallax_numLayers;
    // Corrects for Z view angle
    float viewCorrection = (-viewDir.z) + 2.0;
    float stepSize = 1.0 / (float(numSteps) + 1.0);
    vec2 stepOffset = viewDir.xy * vec2(maxHeight, maxHeight) * stepSize;

    vec2 lastOffset = viewDir.xy * vec2(minHeight, minHeight) + uv;
    float lastRayDepth = 1.0;
    float lastHeight = 1.0;

    vec2 p1;
    vec2 p2;
    bool refine = false;

    while (numSteps > 0)
    {
        // Advance ray in direction of TS view direction
        vec2 candidateOffset = mod(lastOffset-stepOffset, 1.0);
        float currentRayDepth = lastRayDepth - stepSize;

        // Sample height map at this offset
        float currentHeight = textureLod(textures[material.normalTexId], candidateOffset, 0.0).r;
        currentHeight = viewCorrection * currentHeight;
        // Test our candidate depth
        if (currentHeight > currentRayDepth)
        {
            p1 = vec2(currentRayDepth, currentHeight);
            p2 = vec2(lastRayDepth, lastHeight);
            // Break if this is the contact refinement pass
            if (refine) {
                lastHeight = currentHeight;
                break;
            // Else, continue raycasting with squared precision
            } else {
                refine = true;
                lastRayDepth = p2.x;
                stepSize /= float(numSteps);
                stepOffset /= float(numSteps);
                continue;
            }
        }
        lastOffset = candidateOffset;
        lastRayDepth = currentRayDepth;
        lastHeight = currentHeight;
        numSteps -= 1;
    }
    // Interpolate between final two points
    float diff1 = p1.x - p1.y;
    float diff2 = p2.x - p2.y;
    float denominator = diff2 - diff1;

    float parallaxAmount;
    if(denominator != 0.0) {
        parallaxAmount = (p1.x * diff2 - p2.x * diff1) / denominator;
    }

    float offset = ((1.0 - parallaxAmount) * -maxHeight) + minHeight;
    return vec3(viewDir.xy * offset + uv, lastHeight);
}

// Parallax shadowing, very expensive method (per-fragment*per-light tangent-space raycast)
float GetParallaxShadow(vec2 uv, vec3 lightDir, vec3 viewDir, float sampleHeight)
{
    float maxDistance = material.heightScale * 0.1;
    float currentHeight =  textureLod(textures[material.normalTexId], uv, 0.0).r;
    vec2 lightDirUV = normalize(lightDir.xy);
    float heightStep = lightDir.z / float(parallax_numLayers);
    float stepSizeUV = maxDistance / float(parallax_numLayers);

    for (int i = 0; i < parallax_numLayers; ++i) {
        uv += lightDirUV * stepSizeUV; // Step across
        currentHeight += heightStep; // Step up
            
        float heightAtSample = 1 - textureLod(textures[material.normalTexId], uv, 0.0).r;
    
        if (heightAtSample > currentHeight) {
            return 0.1;
        }
    }
    
    return 1.0;
}