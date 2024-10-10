vec4 rgbe_to_float(uvec4 col) 
{
    if (col.a == 0u) {
        return vec4(0.0);
    }

    int e = int(col.a) - 128;

    return vec4(
        ldexp((float(col.r) + 0.5) / 256.0, e),
        ldexp((float(col.g) + 0.5) / 256.0, e),
        ldexp((float(col.b) + 0.5) / 256.0, e),
        1.0
    );
}

uvec4 float_to_rgbe(vec3 col) {
    float maxComponent = max(max(col.r, col.g), col.b);

    if (maxComponent < 1e-32) {
        return uvec4(0, 0, 0, 0);
    }

    // Calculate the exponent and normalize the components
    int e;
    float scale = frexp(maxComponent, e);  // scale is in range [0.5, 1.0)
    scale *= 256.0 / maxComponent;

    return clamp(uvec4(
        int(col.r * scale + 0.5),
        int(col.g * scale + 0.5),
        int(col.b * scale + 0.5),
        e + 128), 0, 255 // Add 128 to the exponent to store in 8 bits
    );
}

vec4 to_vec4(uvec4 uvec) {
    return vec4(uvec) / 255.0;
}

uvec4 to_uvec4(vec4 vec) {
    return clamp(uvec4(vec * 255.0), 0, 255);
}

// converts rgbe in unsigned normalized float format to unsigned normalized float format in rgb
vec4 rgbe_f_to_float(vec4 vec)
{
    return rgbe_to_float(to_uvec4(vec));
}

// converts rgb in unsigned normalized float format to unsigned normalized float format in rgbe
vec4 float_f_to_rgbe(vec3 vec)
{
    return to_vec4(float_to_rgbe(vec));
}