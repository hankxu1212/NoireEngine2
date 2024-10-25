#pragma once

#include <glm/glm.hpp>
#include <iostream>
#include <iomanip>

class Mat4
{
public:
	static const glm::mat4 Identity; // the identity matrix

	// Returns true if two matrices are approximately equal
	static bool Compare(const glm::mat4& mat1, const glm::mat4& mat2, float epsilon = 1e-6f);


	//look at matrix:
	// makes a camera-space-from-world matrix for a camera at eye looking toward
	// target with up-vector pointing (as-close-as-possible) along up.
	// That is, it maps:
	//  - eye_xyz to the origin
	//  - the unit length vector from eye_xyz to target_xyz to -z
	//  - an as-close-as-possible unit-length vector to up to +y
	static glm::mat4 LookAt(
		float eye_x, float eye_y, float eye_z,
		float target_x, float target_y, float target_z,
		float up_x, float up_y, float up_z);

	static glm::mat4 LookAt(
		const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up);

	//perspective projection matrix.
	// - vfov is fov *in radians*
	// - near maps to 0, far maps to 1
	// looks down -z with +y up and +x right
	static glm::mat4 Perspective(float vfov, float aspect, float near, float far);

	static void PrettyPrint(const glm::mat4& mat, int precision=2) {
		// Set the precision and alignment for output
		std::cout << std::fixed << std::setprecision(precision);

		// Iterate through each row of the matrix
		for (int row = 0; row < 4; ++row) {
			std::cout << "[ ";
			for (int col = 0; col < 4; ++col) {
				// Print each element with padding
				std::cout << std::setw(8) << mat[col][row] << " ";
			}
			std::cout << "]\n";
		}
	}

};

