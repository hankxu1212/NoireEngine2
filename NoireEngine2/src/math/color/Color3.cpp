#include "Color3.hpp"

const Color3 Color3::White(0xFFFFFF);
const Color3 Color3::Black(0x000000);
const Color3 Color3::Grey(0x808080);
const Color3 Color3::Silver(0xC0C0C0);
const Color3 Color3::Maroon(0x800000);
const Color3 Color3::Red(0xFF0000);
const Color3 Color3::Olive(0x808000);
const Color3 Color3::Yellow(0xFFFF00);
const Color3 Color3::Green(0x00FF00);
const Color3 Color3::Lime(0x008000);
const Color3 Color3::Teal(0x008080);
const Color3 Color3::Aqua(0x00FFFF);
const Color3 Color3::Navy(0x000080);
const Color3 Color3::Blue(0x0000FF);
const Color3 Color3::Purple(0x800080);
const Color3 Color3::Fuchsia(0xFF00FF);

bool Color3::operator==(const Color3& rhs) const {
	return memcmp(rgb, rhs.rgb, sizeof(rgb));
}

bool Color3::operator!=(const Color3& rhs) const {
	return !operator==(rhs);
}

constexpr Color3 operator+(const Color3& lhs, const Color3& rhs) {
	return { lhs.rgb[0] + rhs.rgb[0], lhs.rgb[1] + rhs.rgb[1], lhs.rgb[2] + rhs.rgb[2] };
}

constexpr Color3 operator-(const Color3& lhs, const Color3& rhs) {
	return { lhs.rgb[0] - rhs.rgb[0], lhs.rgb[1] - rhs.rgb[1], lhs.rgb[2] - rhs.rgb[2] };
}

constexpr Color3 operator*(const Color3& lhs, const Color3& rhs) {
	return { lhs.rgb[0] * rhs.rgb[0], lhs.rgb[1] * rhs.rgb[1], lhs.rgb[2] * rhs.rgb[2] };
}

constexpr Color3 operator/(const Color3& lhs, const Color3& rhs) {
	return { lhs.rgb[0] / rhs.rgb[0], lhs.rgb[1] / rhs.rgb[1], lhs.rgb[2] / rhs.rgb[2] };
}

constexpr Color3 operator+(float lhs, const Color3& rhs) {
	return Color3(lhs, lhs, lhs) + rhs;
}

constexpr Color3 operator-(float lhs, const Color3& rhs) {
	return Color3(lhs, lhs, lhs) - rhs;
}

constexpr Color3 operator*(float lhs, const Color3& rhs) {
	return Color3(lhs, lhs, lhs) * rhs;
}

constexpr Color3 operator/(float lhs, const Color3& rhs) {
	return Color3(lhs, lhs, lhs) / rhs;
}

constexpr Color3 operator+(const Color3& lhs, float rhs) {
	return lhs + Color3(rhs, rhs, rhs);
}

constexpr Color3 operator-(const Color3& lhs, float rhs) {
	return lhs - Color3(rhs, rhs, rhs);
}

constexpr Color3 operator*(const Color3& lhs, float rhs) {
	return lhs * Color3(rhs, rhs, rhs);
}

constexpr Color3 operator/(const Color3& lhs, float rhs) {
	return lhs / Color3(rhs, rhs, rhs);
}

constexpr Color3& Color3::operator+=(const Color3& rhs) {
	return *this = *this + rhs;
}

constexpr Color3& Color3::operator-=(const Color3& rhs) {
	return *this = *this - rhs;
}

constexpr Color3& Color3::operator*=(const Color3& rhs) {
	return *this = *this * rhs;
}

constexpr Color3& Color3::operator/=(const Color3& rhs) {
	return *this = *this / rhs;
}

constexpr Color3& Color3::operator+=(float rhs) {
	return *this = *this + rhs;
}

constexpr Color3& Color3::operator-=(float rhs) {
	return *this = *this - rhs;
}

constexpr Color3& Color3::operator*=(float rhs) {
	return *this = *this * rhs;
}

constexpr Color3& Color3::operator/=(float rhs) {
	return *this = *this / rhs;
}

namespace std {
template<>
struct hash<Color3> {
	size_t operator()(const Color3& colour) const noexcept {
		size_t seed = 0;
		Math::HashCombine(seed, colour.rgb[0]);
		Math::HashCombine(seed, colour.rgb[1]);
		Math::HashCombine(seed, colour.rgb[2]);
		return seed;
	}
};
