struct hitPayload
{
    vec3 hitValue;
    int  depth;
    vec3 rayOrigin;
    int  done;
    vec3 rayDir;
    uint seed;
};

layout(push_constant) uniform constants 
{ 
    uint rayDepth;
    int frame;
} rayConstants;