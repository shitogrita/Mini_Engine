#pragma once

#include <array>

struct Vec3 {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

using Matrix4 = std::array<std::array<float, 4>, 4>;