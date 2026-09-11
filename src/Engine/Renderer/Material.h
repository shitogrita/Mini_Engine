#pragma once

#include "Engine/Math/matrix_types.h"

class Material {
public:
	const Vec3& GetColor() const;
	void SetColor(const Vec3& color);

private:
	Vec3 color_{1.0f, 1.0f, 1.0f};
};