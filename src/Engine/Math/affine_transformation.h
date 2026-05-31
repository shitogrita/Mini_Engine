#pragma once

#include "matrix_types.h"

#include <array>

class AffineTransformation {
public:
	static Matrix4 Identity4();

	static Matrix4 Translation4(float dx, float dy, float dz);

	static Matrix4 GetRotationXMatrix(float degrees);
	static Matrix4 GetRotationYMatrix(float degrees);
	static Matrix4 GetRotationZMatrix(float degrees);

	static Matrix4 Scale(float sx, float sy, float sz);
	static Matrix4 Scale(float value);

	static Matrix4 BuildAffine4(const Matrix4& rotation_or_scale,
								const Vec3& translation);

	static Matrix4 Multiply4(const Matrix4& lhs, const Matrix4& rhs);

	static Matrix4 ComposeModelMatrix(const Vec3& position,
									  const Vec3& rotation_degrees,
									  const Vec3& scale);

	static std::array<float, 16> GetColMajor(const Matrix4& matrix);
};