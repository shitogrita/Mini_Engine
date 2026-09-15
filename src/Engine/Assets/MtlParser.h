#pragma once

#include "Engine/Assets/ImportedMaterialData.h"

#include <filesystem>
#include <vector>

/**
 * @brief Парсер файлов материалов формата MTL.
 *
 * Читает описание материалов из .mtl файла
 * и преобразует их в ImportedMaterialData.
 *
 * Класс не создаёт OpenGL-ресурсы и не загружает текстуры.
 */
class MtlParser {
public:
	static std::vector<ImportedMaterialData> Parse(const std::filesystem::path& path);
};