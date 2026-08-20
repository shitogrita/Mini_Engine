#pragma once

#include "../Math/matrix_types.h"

struct Vec2 {
	float x = 0.0f;
	float y = 0.0f;
};

struct Color3 {
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
};

struct Vertex {
	Vec3 position{};
	Vec3 normal{};
	Vec2 tex_coord{};
	Color3 color{};
};