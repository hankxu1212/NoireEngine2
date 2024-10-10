#pragma once

#include "Color3.hpp"
#include "Color4.hpp"

struct Color4_4 
{ 
	uint8_t r, g, b, a; 

	static const Color4_4 Green;
	static const Color4_4 White;
	static const Color4_4 Yellow;
	static const Color4_4 Blue;
};

inline glm::vec4 rgbe_to_float(glm::u8vec4 col) {
	//avoid decoding zero to a denormalized value:
	if (col.a == 0) return glm::vec4(0.0f);

	int exp = int(col.a) - 128;
	return glm::vec4(
		std::ldexp((col.r + 0.5f) / 256.0f, exp),
		std::ldexp((col.g + 0.5f) / 256.0f, exp),
		std::ldexp((col.b + 0.5f) / 256.0f, exp),
		1
	);
}

inline glm::u8vec4 float_to_rgbe(const glm::vec4& col) 
{
    float maxComponent = glm::max(glm::max(col.r, col.g), col.b);

    if (maxComponent < 1e-32f)
        return glm::u8vec4(0, 0, 0, 0);

    // Calculate the exponent and normalize the components
    int exp;
    float scale = frexp(maxComponent, &exp);  // scale is in range [0.5, 1.0)
    scale *= 256.0f / maxComponent;

    return glm::u8vec4(
        glm::clamp(int(col.r * scale + 0.5f), 0, 255),
        glm::clamp(int(col.g * scale + 0.5f), 0, 255),
        glm::clamp(int(col.b * scale + 0.5f), 0, 255),
        glm::clamp(exp + 128, 0, 255)  // Add 128 to the exponent to store in 8 bits
    );
}