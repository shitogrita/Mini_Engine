#pragma once

#include "Engine/Math/matrix_types.h"

/**
 * @brief Точечный источник света.
 *
 * Излучает свет из одной позиции во всех направлениях.
 * На данном этапе хранит только базовые параметры,
 * необходимые для простого освещения.
 */
class PointLight {
public:
	PointLight() = default;

	PointLight(const Vec3& position, const Vec3& color)
		: position_(position),
		  color_(color) {
	}

	const Vec3& GetPosition() const {
		return position_;
	}

	void SetPosition(const Vec3& position) {
		position_ = position;
	}

	const Vec3& GetColor() const {
		return color_;
	}

	void SetColor(const Vec3& color) {
		color_ = color;
	}

	float GetIntensity() const {
		return intensity_;
	}

	void SetIntensity(float intensity) {
		intensity_ = intensity;
	}

private:
	Vec3 position_{1.2f, 1.0f, 2.0f};
	Vec3 color_{1.0f, 1.0f, 1.0f};
	float intensity_{1.0f};
};