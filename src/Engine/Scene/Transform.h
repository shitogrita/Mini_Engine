#pragma once

#include "../Math/matrix_types.h"


struct Transform {
	Vec3 position{
		0.0f,
		0.0f,
		0.0f
	};

	Vec3 rotation{
		0.0f,
		0.0f,
		0.0f
	};

	Vec3 scale{
		1.0f,
		1.0f,
		1.0f
	};

	Matrix4 GetModelMatrix() const;
};