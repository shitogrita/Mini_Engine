#include "projection.h"

#include <cmath>

namespace {
	constexpr float kPi = 3.14159265358979323846f;
}

Matrix4 Projection::Perspective(float fov_y_deg,
								float aspect,
								float z_near,
								float z_far) {
	const float fov_rad = fov_y_deg * kPi / 180.0f;
	const float f = 1.0f / std::tan(fov_rad * 0.5f);

	Matrix4 p{};

	p[0][0] = f / aspect;
	p[0][1] = 0.0f;
	p[0][2] = 0.0f;
	p[0][3] = 0.0f;

	p[1][0] = 0.0f;
	p[1][1] = f;
	p[1][2] = 0.0f;
	p[1][3] = 0.0f;

	p[2][0] = 0.0f;
	p[2][1] = 0.0f;
	p[2][2] = (z_far + z_near) / (z_near - z_far);
	p[2][3] = (2.0f * z_far * z_near) / (z_near - z_far);

	p[3][0] = 0.0f;
	p[3][1] = 0.0f;
	p[3][2] = -1.0f;
	p[3][3] = 0.0f;

	return p;
}

Matrix4 Projection::Ortho(float left,
						  float right,
						  float bottom,
						  float top,
						  float z_near,
						  float z_far) {
	Matrix4 o{};

	o[0][0] =  2.0f / (right - left);
	o[0][1] =  0.0f;
	o[0][2] =  0.0f;
	o[0][3] = -(right + left) / (right - left);

	o[1][0] =  0.0f;
	o[1][1] =  2.0f / (top - bottom);
	o[1][2] =  0.0f;
	o[1][3] = -(top + bottom) / (top - bottom);

	o[2][0] =  0.0f;
	o[2][1] =  0.0f;
	o[2][2] = -2.0f / (z_far - z_near);
	o[2][3] = -(z_far + z_near) / (z_far - z_near);

	o[3][0] = 0.0f;
	o[3][1] = 0.0f;
	o[3][2] = 0.0f;
	o[3][3] = 1.0f;

	return o;
}

Matrix4 Projection::OrthoSymmetric(float half_w,
								   float half_h,
								   float z_near,
								   float z_far) {
	return Ortho(-half_w, +half_w, -half_h, +half_h, z_near, z_far);
}