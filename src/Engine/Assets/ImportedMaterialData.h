#pragma once

#include "Engine/Math/matrix_types.h"

#include <filesystem>
#include <string>

/**
 * @brief Данные материала, прочитанные из MTL-файла.
 *
 * Здесь хранятся только данные импорта.
 * OpenGL-ресурсов и Texture2D эта структура не содержит.
 */
struct ImportedMaterialData {
	std::string name;
	Vec3 diffuse_color{1.0f, 1.0f, 1.0f};
	float shininess = 32.0f;
	std::filesystem::path diffuse_texture_path;
};