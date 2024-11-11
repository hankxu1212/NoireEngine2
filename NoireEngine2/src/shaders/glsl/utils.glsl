#ifndef _INCLUDE_UTILS
#define _INCLUDE_UTILS

#define saturate(x) clamp(x, 0.0, 1.0)

const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

vec3 LinearToSRGB(vec3 color)
{
    return pow(color, vec3(INV_GAMMA));
}

vec3 SRGBToLinear(vec3 srgbIn)
{
    return vec3(pow(srgbIn.xyz, vec3(GAMMA)));
}

// Mathematical constants
#define PI              3.14159265359        // Pi
#define TWO_PI          6.28318530718        // 2 * Pi
#define FOUR_PI         12.5663706144        // 2 * Pi
#define HALF_PI         1.57079632679        // Pi / 2
#define QUARTER_PI      0.78539816339        // Pi / 4
#define EIGHTH_PI       0.39269908169        // Pi / 8
#define SIXTEENTH_PI    0.19634954084        // Pi / 16
#define THIRD_PI        1.0471975512         // Pi / 3
#define SIXTH_PI        0.52359877559        // Pi / 6

#define INV_PI          0.31830988618        // 1 / Pi
#define INV_TWO_PI      0.15915494309        // 1 / (2 * Pi)

#define DEG_TO_RAD      0.01745329252        // Degrees to radians conversion factor (Pi / 180)
#define RAD_TO_DEG      57.2957795131        // Radians to degrees conversion factor (180 / Pi)

#define SQRT_TWO        1.41421356237        // Square root of 2
#define SQRT_THREE      1.73205080757        // Square root of 3
#define EPSILON         1e-5                 // Small epsilon value for floating point comparisons

// Compute orthonormal basis for converting from tanget/shading space to world space.
void ComputeTangentBitangent(const vec3 N, out vec3 T, out vec3 B)
{
	vec3 reference = (abs(N.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(N, reference));
    B = normalize(cross(N, T));
}

// Convert point from tangent/shading space to world space.
vec3 TangentToWorld(const vec3 v, const vec3 T, const vec3 N, const vec3 B)
{
	return T * v.x + B * v.y + N * v.z;
}

vec3 Uncharted2Tonemap(vec3 x) {
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  float W = 11.2;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 ACES(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float ACES(float x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

#define C_Stack_Max 3.402823466e+38f
uint CompressUnitVec(vec3 nv)
{
  // map to octahedron and then flatten to 2D (see 'Octahedron Environment Maps' by Engelhardt & Dachsbacher)
  if((nv.x < C_Stack_Max) && !isinf(nv.x))
  {
    const float d = 32767.0f / (abs(nv.x) + abs(nv.y) + abs(nv.z));
    int         x = int(roundEven(nv.x * d));
    int         y = int(roundEven(nv.y * d));
    if(nv.z < 0.0f)
    {
      const int maskx = x >> 31;
      const int masky = y >> 31;
      const int tmp   = 32767 + maskx + masky;
      const int tmpx  = x;
      x               = (tmp - (y ^ masky)) ^ maskx;
      y               = (tmp - (tmpx ^ maskx)) ^ masky;
    }
    uint packed = (uint(y + 32767) << 16) | uint(x + 32767);
    if(packed == ~0u)
      return ~0x1u;
    return packed;
  }
  else
  {
    return ~0u;
  }
}

float ShortToFloatM11(const int v)  // linearly maps a short 32767-32768 to a float -1-+1 //!! opt.?
{
  return (v >= 0) ? (uintBitsToFloat(0x3F800000u | (uint(v) << 8)) - 1.0f) :
                    (uintBitsToFloat((0x80000000u | 0x3F800000u) | (uint(-v) << 8)) + 1.0f);
}

vec3 DecompressUnitVec(uint packed)
{
  if(packed != ~0u)  // sanity check, not needed as isvalid_unit_vec is called earlier
  {
    int       x     = int(packed & 0xFFFFu) - 32767;
    int       y     = int(packed >> 16) - 32767;
    const int maskx = x >> 31;
    const int masky = y >> 31;
    const int tmp0  = 32767 + maskx + masky;
    const int ymask = y ^ masky;
    const int tmp1  = tmp0 - (x ^ maskx);
    const int z     = tmp1 - ymask;
    float     zf;
    if(z < 0)
    {
      x  = (tmp0 - ymask) ^ maskx;
      y  = tmp1 ^ masky;
      zf = uintBitsToFloat((0x80000000u | 0x3F800000u) | (uint(-z) << 8)) + 1.0f;
    }
    else
    {
      zf = uintBitsToFloat(0x3F800000u | (uint(z) << 8)) - 1.0f;
    }
    return normalize(vec3(ShortToFloatM11(x), ShortToFloatM11(y), zf));
  }
  else
  {
    return vec3(C_Stack_Max);
  }
}


//-------------------------------------------------------------------------------------------------
// Avoiding self intersections (see Ray Tracing Gems, Ch. 6)
//
vec3 OffsetRay(in vec3 p, in vec3 n)
{
  const float intScale   = 256.0f;
  const float floatScale = 1.0f / 65536.0f;
  const float origin     = 1.0f / 32.0f;

  ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

  vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
                  intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
                  intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

  return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,  //
              abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,  //
              abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}


//////////////////////////// AO //////////////////////////////////////
#define EPS 0.05
const float M_PI = 3.141592653589;

void ComputeDefaultBasis(const vec3 normal, out vec3 x, out vec3 y)
{
  // ZAP's default coordinate system for compatibility
  vec3        z  = normal;
  const float yz = -z.y * z.z;
  y = normalize(((abs(z.z) > 0.99999f) ? vec3(-z.x * z.y, 1.0f - z.y * z.y, yz) : vec3(-z.x * z.z, yz, 1.0f - z.z * z.z)));

  x = cross(y, z);
}

#endif