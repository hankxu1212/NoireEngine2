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
    vec4 clearColor;
    uint rayDepth;
} rayConstants;