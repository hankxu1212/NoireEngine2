#include "Mat4.h"

constexpr glm::mat4 Mat4::Identity = glm::mat4(1);


glm::mat4 Mat4::Perspective(float vfov, float aspect, float near, float far) {
	//as per https://www.terathon.com/gdc07_lengyel.pdf
	// (with modifications for Vulkan-style coordinate system)
	//  notably: flip y (vulkan device coords are y-down)
	//       and rescale z (vulkan device coords are z-[0,1])
	const float e = 1.0f / std::tan(vfov / 2.0f);
	const float a = aspect;
	const float n = near;
	const float f = far;
	return glm::mat4{ //note: column-major storage order!
		e / a,  0.0f,                      0.0f, 0.0f,
		0.0f,   -e,                      0.0f, 0.0f,
		0.0f, 0.0f,-0.5f - 0.5f * (f + n) / (f - n),-1.0f,
		0.0f, 0.0f,             -(f * n) / (f - n), 0.0f,
	};
}

glm::mat4 Mat4::LookAt(
	float eye_x, float eye_y, float eye_z,
	float target_x, float target_y, float target_z,
	float up_x, float up_y, float up_z) {

	//NOTE: this would be a lot cleaner with a vec3 type and some overloads!

	//compute vector from eye to target:
	float in_x = target_x - eye_x;
	float in_y = target_y - eye_y;
	float in_z = target_z - eye_z;

	//normalize 'in' vector:
	float inv_in_len = 1.0f / std::sqrt(in_x * in_x + in_y * in_y + in_z * in_z);
	in_x *= inv_in_len;
	in_y *= inv_in_len;
	in_z *= inv_in_len;

	//make 'up' orthogonal to 'in':
	float in_dot_up = in_x * up_x + in_y * up_y + in_z * up_z;
	up_x -= in_dot_up * in_x;
	up_y -= in_dot_up * in_y;
	up_z -= in_dot_up * in_z;

	//normalize 'up' vector:
	float inv_up_len = 1.0f / std::sqrt(up_x * up_x + up_y * up_y + up_z * up_z);
	up_x *= inv_up_len;
	up_y *= inv_up_len;
	up_z *= inv_up_len;

	//compute 'right' vector as 'in' x 'up'
	float right_x = in_y * up_z - in_z * up_y;
	float right_y = in_z * up_x - in_x * up_z;
	float right_z = in_x * up_y - in_y * up_x;

	//compute dot products of right, in, up with eye:
	float right_dot_eye = right_x * eye_x + right_y * eye_y + right_z * eye_z;
	float up_dot_eye = up_x * eye_x + up_y * eye_y + up_z * eye_z;
	float in_dot_eye = in_x * eye_x + in_y * eye_y + in_z * eye_z;

	//final matrix: (computes (right . (v - eye), up . (v - eye), -in . (v-eye), v.w )
	return glm::mat4{ //note: column-major storage order
		right_x, up_x, -in_x, 0.0f,
		right_y, up_y, -in_y, 0.0f,
		right_z, up_z, -in_z, 0.0f,
		-right_dot_eye, -up_dot_eye, in_dot_eye, 1.0f,
	};
}

bool Mat4::Compare(const glm::mat4& mat1, const glm::mat4& mat2, float epsilon)
{
	// Compare each element of the matrices
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (std::fabs(mat1[i][j] - mat2[i][j]) > epsilon) {
				return false; // Matrices are not equal
			}
		}
	}
	return true; // Matrices are equal
}
