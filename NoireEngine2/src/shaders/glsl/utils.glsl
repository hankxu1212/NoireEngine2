#define saturate(x) clamp(x, 0.0, 1.0)

vec4 gamma_map(vec3 color, float gamma)
{
	color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/ gamma)); 

	return vec4(color, 1);
}

// Mathematical constants
#define PI              3.14159265359        // Pi
#define TWO_PI          6.28318530718        // 2 * Pi
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

void ComputeTangentBitangent(vec3 normal, out vec3 tangent, out vec3 bitangent)
{
    vec3 reference = (abs(normal.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    tangent = normalize(cross(normal, reference));
    bitangent = normalize(cross(normal, tangent));
}