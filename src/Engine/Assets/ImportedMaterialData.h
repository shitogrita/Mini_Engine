#pragma once

#include "Engine/Math/matrix_types.h"

#include <filesystem>
#include <string>

/**
 * @brief Данные материала, прочитанные из файла модели.
 *
 * Структура не содержит OpenGL-ресурсов.
 * Она только хранит информацию, полученную из MTL.
 * Так как mesh хранит у нас только геометрию
 */
struct ImportedMaterialData {
	std::string name;

	Vec3 diffuse_color{1.0f, 1.0f, 1.0f};

	float shininess = 32.0f;

	std::filesystem::path diffuse_texture_path;
};