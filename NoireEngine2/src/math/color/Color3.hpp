#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <glm/gtc/type_ptr.hpp>

#include "math/Math.hpp"

class Color3
{
public:
	float rgb[3] = { 0 };
public:
	Color3() = default;
		
	constexpr Color3(float r, float g, float b) 
	{
		rgb[0] = r;
		rgb[1] = g;
		rgb[2] = b;
	}

	/**
		* Constructor for Color.
		* @param i The integer value.
		* @param type The order components of colour are packed.
		*/
	constexpr Color3(uint32_t i) {
		rgb[0] = static_cast<float>((uint8_t)(i >> 16)) / 255.0f;
		rgb[1] = static_cast<float>((uint8_t)(i >> 8)) / 255.0f;
		rgb[2] = static_cast<float>((uint8_t)(i & 0xFF)) / 255.0f;
	}

	/**
		* Constructor for Color.
		* @param hex The new values from HEX.
		* @param a The new A value.
		*/
	Color3(std::string hex) {
		if (hex[0] == '#') {
			hex.erase(0, 1);
		}

		assert(hex.size() == 6);
		auto hexValue = std::stoul(hex, nullptr, 16);

		rgb[0] = static_cast<float>((hexValue >> 16) & 0xff) / 255.0f;
		rgb[1] = static_cast<float>((hexValue >> 8) & 0xff) / 255.0f;
		rgb[2] = static_cast<float>((hexValue >> 0) & 0xff) / 255.0f;
	}

	/**
		* Calculates the linear interpolation between this colour and another colour.
		* @param other The other quaternion.
		* @param progression The progression.
		* @return Left lerp right.
		*/
	constexpr Color3 Lerp(const Color3& other, float progression) const {
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
	constexpr float NormSquared() const {
		return rgb[0] * rgb[0] + rgb[1] * rgb[1] + rgb[2] * rgb[2];
	}

	/**
		* Gets the length of this colour.
		* @return The length.
		*/
	float Length() const {
		return std::sqrt(NormSquared());
	}

	/**
		* Gradually changes this colour to a target.
		* @param target The target colour.
		* @param rate The rate to go from current to the target.
		* @return The changed colour.
		*/
	constexpr Color3 SmoothDamp(const Color3& target, const Color3& rate) const {
		return Math::SmoothDamp(*this, target, rate);
	}

	/**
		* Gets a colour representing the unit value of this colour.
		* @return The unit colour.
		*/
	Color3 Unit() const {
		auto l = Length();
		return *this / l;
	}

	/**
		* Gets a packed integer representing this colour.
		* @param type The order components of colour are packed.
		* @return The packed integer.
		*/
	constexpr uint32_t ToInt() const {
		return (static_cast<uint8_t>(rgb[0] * 255.0f) << 16) | 
			(static_cast<uint8_t>(rgb[1] * 255.0f) << 8) | 
			(static_cast<uint8_t>(rgb[2] * 255.0f) & 0xFF);
	}

	/**
		* Gets the hex code from this colour.
		* @return The hex code.
		*/
	std::string GetHex() const {
		std::stringstream stream;
		stream << "#";

		auto hexValue = ((static_cast<uint32_t>(rgb[0] * 255.0f) & 0xff) << 16) +
			((static_cast<uint32_t>(rgb[1] * 255.0f) & 0xff) << 8) +
			((static_cast<uint32_t>(rgb[2] * 255.0f) & 0xff) << 0);
		stream << std::hex << std::setfill('0') << std::setw(6) << hexValue;

		return stream.str();
	}

	operator glm::vec3() const {
		return glm::make_vec3(rgb);
	}

	float* value_ptr() {
		return &rgb[0];
	}

	constexpr float operator[](uint32_t i) const {
		return rgb[i];
	}
	constexpr float& operator[](uint32_t i) {
		return rgb[i];
	}

	bool operator==(const Color3& rhs) const;
	bool operator!=(const Color3& rhs) const;

	constexpr friend Color3 operator+(const Color3& lhs, const Color3& rhs);
	constexpr friend Color3 operator-(const Color3& lhs, const Color3& rhs);
	constexpr friend Color3 operator*(const Color3& lhs, const Color3& rhs);
	constexpr friend Color3 operator/(const Color3& lhs, const Color3& rhs);
	constexpr friend Color3 operator+(float lhs, const Color3& rhs);
	constexpr friend Color3 operator-(float lhs, const Color3& rhs);
	constexpr friend Color3 operator*(float lhs, const Color3& rhs);
	constexpr friend Color3 operator/(float lhs, const Color3& rhs);
	constexpr friend Color3 operator+(const Color3& lhs, float rhs);
	constexpr friend Color3 operator-(const Color3& lhs, float rhs);
	constexpr friend Color3 operator*(const Color3& lhs, float rhs);
	constexpr friend Color3 operator/(const Color3& lhs, float rhs);

	constexpr Color3& operator+=(const Color3& rhs);
	constexpr Color3& operator-=(const Color3& rhs);
	constexpr Color3& operator*=(const Color3& rhs);
	constexpr Color3& operator/=(const Color3& rhs);
	constexpr Color3& operator+=(float rhs);
	constexpr Color3& operator-=(float rhs);
	constexpr Color3& operator*=(float rhs);
	constexpr Color3& operator/=(float rhs);

	static const Color3 White;
	static const Color3 Black;
	static const Color3 Grey;
	static const Color3 Silver;
	static const Color3 Maroon;
	static const Color3 Red;
	static const Color3 Olive;
	static const Color3 Yellow;
	static const Color3 Green;
	static const Color3 Lime;
	static const Color3 Teal;
	static const Color3 Aqua;
	static const Color3 Navy;
	static const Color3 Blue;
	static const Color3 Purple;
	static const Color3 Fuchsia;

};

