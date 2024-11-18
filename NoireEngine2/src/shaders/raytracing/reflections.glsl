struct hitPayload
{
    vec3 hitValue;
    int  depth;
    vec3 attenuation;
    int  done;
    vec3 rayOrigin;
    vec3 rayDir;
};

layout(push_constant) uniform constants 
{ 
    uint rayDepth;
} rayConstants;