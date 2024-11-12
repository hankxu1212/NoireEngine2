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

void main()
{
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;
    
    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(emissiveMap[mipLevel], vec2(fragUV.x - 2*x, fragUV.y + 2*y)).rgb;
    vec3 b = texture(emissiveMap[mipLevel], vec2(fragUV.x,       fragUV.y + 2*y)).rgb;
    vec3 c = texture(emissiveMap[mipLevel], vec2(fragUV.x + 2*x, fragUV.y + 2*y)).rgb;
    
    vec3 d = texture(emissiveMap[mipLevel], vec2(fragUV.x - 2*x, fragUV.y)).rgb;
    vec3 e = texture(emissiveMap[mipLevel], vec2(fragUV.x,       fragUV.y)).rgb;
    vec3 f = texture(emissiveMap[mipLevel], vec2(fragUV.x + 2*x, fragUV.y)).rgb;
    
    vec3 g = texture(emissiveMap[mipLevel], vec2(fragUV.x - 2*x, fragUV.y - 2*y)).rgb;
    vec3 h = texture(emissiveMap[mipLevel], vec2(fragUV.x,       fragUV.y - 2*y)).rgb;
    vec3 i = texture(emissiveMap[mipLevel], vec2(fragUV.x + 2*x, fragUV.y - 2*y)).rgb;
    
    vec3 j = texture(emissiveMap[mipLevel], vec2(fragUV.x - x, fragUV.y + y)).rgb;
    vec3 k = texture(emissiveMap[mipLevel], vec2(fragUV.x + x, fragUV.y + y)).rgb;
    vec3 l = texture(emissiveMap[mipLevel], vec2(fragUV.x - x, fragUV.y - y)).rgb;
    vec3 m = texture(emissiveMap[mipLevel], vec2(fragUV.x + x, fragUV.y - y)).rgb;
    
    outEmissive.rgb = e*0.125;
    outEmissive.rgb += (a+c+g+i)*0.03125;
    outEmissive.rgb += (b+d+f+h)*0.0625;
    outEmissive.rgb += (j+k+l+m)*0.125;
    outEmissive = vec4(outEmissive.r, outEmissive.g, outEmissive.b, 1.0);
}