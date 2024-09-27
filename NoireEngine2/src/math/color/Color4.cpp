#include "Color4.hpp"

const Color4 Color4::Clear(0x00000000);
const Color4 Color4::Black(0x000000FF);
const Color4 Color4::Grey(0x808080);
const Color4 Color4::Silver(0xC0C0C0);
const Color4 Color4::White(0xFFFFFF);
const Color4 Color4::Maroon(0x800000);
const Color4 Color4::Red(0xFF0000);
const Color4 Color4::Olive(0x808000);
const Color4 Color4::Yellow(0xFFFF00);
const Color4 Color4::Green(0x00FF00);
const Color4 Color4::Lime(0x008000);
const Color4 Color4::Teal(0x008080);
const Color4 Color4::Aqua(0x00FFFF);
const Color4 Color4::Navy(0x000080);
const Color4 Color4::Blue(0x0000FF);
const Color4 Color4::Purple(0x800080);
const Color4 Color4::Fuchsia(0xFF00FF);	
	
Color4::Color4(const Color3& color3)
{
	r = color3[0];
	g = color3[1];
	b = color3[2];
}

constexpr bool Color4::operator==(const Color4& rhs) const {
	return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

constexpr bool Color4::operator!=(const Color4& rhs) const {
	return !operator==(rhs);
}

constexpr Color4 operator+(const Color4& lhs, const Color4& rhs) {
	return { lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a };
}

constexpr Color4 operator-(const Color4& lhs, const Color4& rhs) {
	return { lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a };
}

constexpr Color4 operator*(const Color4& lhs, const Color4& rhs) {
	return { lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a };
}

constexpr Color4 operator/(const Color4& lhs, const Color4& rhs) {
	return { lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b, lhs.a / rhs.a };
}

constexpr Color4 operator+(float lhs, const Color4& rhs) {
	return Color4(lhs, lhs, lhs, 0.0f) + rhs;
}

constexpr Color4 operator-(float lhs, const Color4& rhs) {
	return Color4(lhs, lhs, lhs, 0.0f) - rhs;
}

constexpr Color4 operator*(float lhs, const Color4& rhs) {
	return Color4(lhs, lhs, lhs) * rhs;
}

constexpr Color4 operator/(float lhs, const Color4& rhs) {
	return Color4(lhs, lhs, lhs) / rhs;
}

constexpr Color4 operator+(const Color4& lhs, float rhs) {
	return lhs + Color4(rhs, rhs, rhs, 0.0f);
}

constexpr Color4 operator-(const Color4& lhs, float rhs) {
	return lhs - Color4(rhs, rhs, rhs, 0.0f);
}

constexpr Color4 operator*(const Color4& lhs, float rhs) {
	return lhs * Color4(rhs, rhs, rhs);
}

constexpr Color4 operator/(const Color4& lhs, float rhs) {
	return lhs / Color4(rhs, rhs, rhs);
}

constexpr Color4& Color4::operator+=(const Color4& rhs) {
	return *this = *this + rhs;
}

constexpr Color4& Color4::operator-=(const Color4& rhs) {
	return *this = *this - rhs;
}

constexpr Color4& Color4::operator*=(const Color4& rhs) {
	return *this = *this * rhs;
}

constexpr Color4& Color4::operator/=(const Color4& rhs) {
	return *this = *this / rhs;
}

constexpr Color4& Color4::operator+=(float rhs) {
	return *this = *this + rhs;
}

constexpr Color4& Color4::operator-=(float rhs) {
	return *this = *this - rhs;
}

constexpr Color4& Color4::operator*=(float rhs) {
	return *this = *this * rhs;
}

constexpr Color4& Color4::operator/=(float rhs) {
	return *this = *this / rhs;
}

inline std::ostream& operator<<(std::ostream& stream, const Color4& colour) {
	return stream << colour.r << ", " << colour.g << ", " << colour.b << ", " << colour.a;
}


namespace std {
	template<>
	struct hash<Color4> {
		size_t operator()(const Color4& colour) const noexcept {
			size_t seed = 0;
			Math::HashCombine(seed, colour.r);
			Math::HashCombine(seed, colour.g);
			Math::HashCombine(seed, colour.b);
			Math::HashCombine(seed, colour.a);
			return seed;
		}
	};
}

