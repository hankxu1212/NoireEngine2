#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D emissiveMap[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outEmissive;

layout(push_constant) uniform Bloom
{
    vec2 srcTexelSize;
    float filterRadius;
    uint mipLevel;
};

const float avg = 1.0 / 16.0;

void main()
{
    float x = filterRadius;
    float y = filterRadius;
    
    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(emissiveMap[mipLevel], vec2(fragUV.x - x, fragUV.y + y)).rgb;
    vec3 b = texture(emissiveMap[mipLevel], vec2(fragUV.x,     fragUV.y + y)).rgb;
    vec3 c = texture(emissiveMap[mipLevel], vec2(fragUV.x + x, fragUV.y + y)).rgb;
    
    vec3 d = texture(emissiveMap[mipLevel], vec2(fragUV.x - x, fragUV.y)).rgb;
    vec3 e = texture(emissiveMap[mipLevel], vec2(fragUV.x,     fragUV.y)).rgb;
    vec3 f = texture(emissiveMap[mipLevel], vec2(fragUV.x + x, fragUV.y)).rgb;
    
    vec3 g = texture(emissiveMap[mipLevel], vec2(fragUV.x - x, fragUV.y - y)).rgb;
    vec3 h = texture(emissiveMap[mipLevel], vec2(fragUV.x,     fragUV.y - y)).rgb;
    vec3 i = texture(emissiveMap[mipLevel], vec2(fragUV.x + x, fragUV.y - y)).rgb;
    
    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    outEmissive.rgb = e * 4.0;
    outEmissive.rgb += (b+d+f+h) * 2.0;
    outEmissive.rgb += (a+c+g+i);
    outEmissive.rgb *= avg;
    
    outEmissive = vec4(outEmissive.r, outEmissive.g, outEmissive.b, 1.0);
}