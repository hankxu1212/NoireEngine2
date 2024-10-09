#define saturate(x) clamp(x, 0.0, 1.0)

vec4 gamma_map(vec3 color, float gamma)
{
	color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/ gamma)); 

	return vec4(color, 1);
}