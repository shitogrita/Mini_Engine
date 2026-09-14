#pragma once

#include "Engine/Math/matrix_types.h"

#include <memory>

/**
 * @brief Описывает визуальные свойства поверхности объекта.
 *
 * Material не содержит геометрию объекта и не является источником света.
 * Он определяет то, как поверхность взаимодействует с освещением.
 * Color             базовый цвет
 * AmbientStrength   сколько фонового света принимает
 * DiffuseStrength   насколько сильно реагирует на прямой свет
 * SpecularStrength  сила блика
 * Shininess         размер/резкость блика
 */

class Texture2D;

class Material {
public:

	void SetDiffuseTexture(std::shared_ptr<Texture2D> texture) { diffuse_texture_ = std::move(texture); }

	const std::shared_ptr<Texture2D>& GetDiffuseTexture() const { return diffuse_texture_; }

	bool HasDiffuseTexture() const { return diffuse_texture_ != nullptr; }

	const Vec3& GetColor() const { return color_; }
	void SetColor(const Vec3& color) { color_ = color; }

	float GetAmbientStrength() const { return ambient_strength_; }
	void SetAmbientStrength(float value) { ambient_strength_ = value; }

	float GetDiffuseStrength() const { return diffuse_strength_; }
	void SetDiffuseStrength(float value) { diffuse_strength_ = value; }

	float GetSpecularStrength() const { return specular_strength_; }
	void SetSpecularStrength(float value) { specular_strength_ = value; }

	float GetShininess() const { return shininess_; }
	void SetShininess(float value) { shininess_ = value; }

private:
	Vec3 color_{0.67f, 0.76f, 0.91f};

	float ambient_strength_{0.15f};
	float diffuse_strength_{1.0f};
	float specular_strength_{0.5f};
	float shininess_{32.0f};

	std::shared_ptr<Texture2D> diffuse_texture_;
};