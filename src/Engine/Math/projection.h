#pragma once

#include "matrix_types.h"

class Projection {
public:
	static Matrix4 Perspective(float fov_y_deg,
							   float aspect,
							   float z_near,
							   float z_far);

	static Matrix4 Ortho(float left,
						 float right,
						 float bottom,
						 float top,
						 float z_near,
						 float z_far);

	static Matrix4 OrthoSymmetric(float half_w,
								  float half_h,
								  float z_near,
								  float z_far);
};