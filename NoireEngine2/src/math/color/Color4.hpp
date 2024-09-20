#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "math/Math.hpp"

class Color4 {
public:
	Color4() = default;

	/**
		* Constructor for Color.
		*/
	constexpr Color4(float r, float g, float b, float a = 1.0f) :
		r(r),
		g(g),
		b(b),
		a(a) {
	}

	operator glm::vec3() const
	{
		return glm::vec3(r, g, b);
	}

	/**
		* Constructor for Color.
		* @param i The integer value.
		* @param type The order components of colour are packed.
		*/
	constexpr Color4(uint32_t i) {
		r = static_cast<float>((uint8_t)(i >> 24 & 0xFF)) / 255.0f;
		g = static_cast<float>((uint8_t)(i >> 16 & 0xFF)) / 255.0f;
		b = static_cast<float>((uint8_t)(i >> 8 & 0xFF)) / 255.0f;
		a = static_cast<float>((uint8_t)(i & 0xFF)) / 255.0f;
	}

	/**
		* Constructor for Color.
		* @param hex The new values from HEX.
		* @param a The new A value.
		*/
	Color4(std::string hex, float a = 1.0f) :
		a(a) {
		if (hex[0] == '#') {
			hex.erase(0, 1);
		}

		assert(hex.size() == 6);
		auto hexValue = std::stoul(hex, nullptr, 16);

		r = static_cast<float>((hexValue >> 16) & 0xff) / 255.0f;
		g = static_cast<float>((hexValue >> 8) & 0xff) / 255.0f;
		b = static_cast<float>((hexValue >> 0) & 0xff) / 255.0f;
	}

	/**
		* Calculates the linear interpolation between this colour and another colour.
		* @param other The other quaternion.
		* @param progression The progression.
		* @return Left lerp right.
		*/
	constexpr Color4 Lerp(const Color4& other, float progression) const {
		auto ta = *this * (1.0f - progression);
		auto tb = other * progression;
		return ta + tb;
	}

	/**
		* Normalizes this colour.
		* @return The normalized colour.
		*/
	void Normalize() {
		auto l = Length();

		if (l == 0.0f)
			throw std::runtime_error("Can't normalize a zero length vector");

		*this /= l;
	}

	/**
		* Gets the length squared of this colour.
		* @return The length squared.
		*/
	constexpr float Length2() const {
		return r * r + g * g + b * b + a * a;
	}

	/**
		* Gets the length of this colour.
		* @return The length.
		*/
	float Length() const {
		return std::sqrt(Length2());
	}

	/**
		* Gradually changes this colour to a target.
		* @param target The target colour.
		* @param rate The rate to go from current to the target.
		* @return The changed colour.
		*/
	constexpr Color4 SmoothDamp(const Color4& target, const Color4& rate) const {
		return Math::SmoothDamp(*this, target, rate);
	}

	/**
		* Gets a colour representing the unit value of this colour.
		* @return The unit colour.
		*/
	Color4 GetUnit() const {
		auto l = Length();
		return { r / l, g / l, b / l, a / l };
	}

	/**
		* Gets a packed integer representing this colour.
		* @param type The order components of colour are packed.
		* @return The packed integer.
		*/
	constexpr uint32_t GetInt() const {
		return (static_cast<uint8_t>(r * 255.0f) << 24) 
			| (static_cast<uint8_t>(g * 255.0f) << 16) 
			| (static_cast<uint8_t>(b * 255.0f) << 8) 
			| (static_cast<uint8_t>(a * 255.0f) & 0xFF);
	}

	/**
		* Gets the hex code from this colour.
		* @return The hex code.
		*/
	std::string GetHex() const {
		std::stringstream stream;
		stream << "#";

		auto hexValue = ((static_cast<uint32_t>(r * 255.0f) & 0xff) << 16) +
			((static_cast<uint32_t>(g * 255.0f) & 0xff) << 8) +
			((static_cast<uint32_t>(b * 255.0f) & 0xff) << 0);
		stream << std::hex << std::setfill('0') << std::setw(6) << hexValue;

		return stream.str();
	}

	constexpr float operator[](uint32_t i) const {
		assert(i < 4 && "Color subscript out of range");
		return i == 0 ? r : i == 1 ? g : i == 2 ? b : a;
	}
	constexpr float& operator[](uint32_t i) {
		assert(i < 4 && "Color subscript out of range");
		return i == 0 ? r : i == 1 ? g : i == 2 ? b : a;
	}

	constexpr bool operator==(const Color4& rhs) const;
	constexpr bool operator!=(const Color4& rhs) const;

	constexpr friend Color4 operator+(const Color4& lhs, const Color4& rhs);
	constexpr friend Color4 operator-(const Color4& lhs, const Color4& rhs);
	constexpr friend Color4 operator*(const Color4& lhs, const Color4& rhs);
	constexpr friend Color4 operator/(const Color4& lhs, const Color4& rhs);
	constexpr friend Color4 operator+(float lhs, const Color4& rhs);
	constexpr friend Color4 operator-(float lhs, const Color4& rhs);
	constexpr friend Color4 operator*(float lhs, const Color4& rhs);
	constexpr friend Color4 operator/(float lhs, const Color4& rhs);
	constexpr friend Color4 operator+(const Color4& lhs, float rhs);
	constexpr friend Color4 operator-(const Color4& lhs, float rhs);
	constexpr friend Color4 operator*(const Color4& lhs, float rhs);
	constexpr friend Color4 operator/(const Color4& lhs, float rhs);

	constexpr Color4& operator+=(const Color4& rhs);
	constexpr Color4& operator-=(const Color4& rhs);
	constexpr Color4& operator*=(const Color4& rhs);
	constexpr Color4& operator/=(const Color4& rhs);
	constexpr Color4& operator+=(float rhs);
	constexpr Color4& operator-=(float rhs);
	constexpr Color4& operator*=(float rhs);
	constexpr Color4& operator/=(float rhs);

	friend std::ostream& operator<<(std::ostream& stream, const Color4& colour);

	static const Color4 Clear;
	static const Color4 Black;
	static const Color4 Grey;
	static const Color4 Silver;
	static const Color4 White;
	static const Color4 Maroon;
	static const Color4 Red;
	static const Color4 Olive;
	static const Color4 Yellow;
	static const Color4 Green;
	static const Color4 Lime;
	static const Color4 Teal;
	static const Color4 Aqua;
	static const Color4 Navy;
	static const Color4 Blue;
	static const Color4 Purple;
	static const Color4 Fuchsia;

	float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
};
