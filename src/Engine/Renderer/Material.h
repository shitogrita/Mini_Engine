#pragma once

#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Texture2D.h"

#include <memory>
#include <filesystem>

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

class Material {
public:
	Material() = default;

	const Vec3& GetColor() const;
	void SetColor(const Vec3& color);

	float GetAmbientStrength() const;
	void SetAmbientStrength(float value);

	float GetDiffuseStrength() const;
	void SetDiffuseStrength(float value);

	float GetSpecularStrength() const;
	void SetSpecularStrength(float value);

	float GetShininess() const;
	void SetShininess(float value);

	void SetDiffuseTexture(std::shared_ptr<Texture2D> texture);
	const std::shared_ptr<Texture2D>& GetDiffuseTexture() const;
	bool HasDiffuseTexture() const;
	void ClearDiffuseTexture();

	void SetDiffuseTexturePath(std::filesystem::path path);
	const std::filesystem::path& GetDiffuseTexturePath() const;

private:
	Vec3 color_{
		0.67f,
		0.76f,
		0.91f
	};

	float ambient_strength_ = 0.20f;
	float diffuse_strength_ = 1.00f;
	float specular_strength_ = 0.50f;
	float shininess_ = 32.0f;

	std::shared_ptr<Texture2D> diffuse_texture_;
	std::filesystem::path diffuse_texture_path_;
};