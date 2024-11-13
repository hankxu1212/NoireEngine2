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

vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0 / 2.2;
vec3 ToSRGB(vec3 v) { return PowVec3(v, invGamma); }

float RGBToLuminance(vec3 col)
{
    return dot(col, vec3(0.2126f, 0.7152f, 0.0722f));
}

float KarisAverage(vec3 col)
{
    // Formula is 1 / (1 + luma)
    float luma = RGBToLuminance(ToSRGB(col)) * 0.25f;
    return 1.0f / (1.0f + luma);
}

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
    
    vec3 groups[5];

    switch (mipLevel)
    {
    case 0:
        // We are writing to mip 0, so we need to apply Karis average to each block
        // of 4 samples to prevent fireflies (very bright subpixels, leads to pulsating
        // artifacts).
        groups[0] = (a+b+d+e) * (0.125f/4.0f);
        groups[1] = (b+c+e+f) * (0.125f/4.0f);
        groups[2] = (d+e+g+h) * (0.125f/4.0f);
        groups[3] = (e+f+h+i) * (0.125f/4.0f);
        groups[4] = (j+k+l+m) * (0.5f/4.0f);
        groups[0] *= KarisAverage(groups[0]);
        groups[1] *= KarisAverage(groups[1]);
        groups[2] *= KarisAverage(groups[2]);
        groups[3] *= KarisAverage(groups[3]);
        groups[4] *= KarisAverage(groups[4]);
        vec3 sum = groups[0]+groups[1]+groups[2]+groups[3]+groups[4];
        outEmissive.rgb = max(sum, 0.0001f);
        break;
    default:
        outEmissive.rgb = e*0.125;
        outEmissive.rgb += (a+c+g+i)*0.03125;
        outEmissive.rgb += (b+d+f+h)*0.0625;
        outEmissive.rgb += (j+k+l+m)*0.125;
        outEmissive = vec4(outEmissive.r, outEmissive.g, outEmissive.b, 1.0);
        break;
    }
}