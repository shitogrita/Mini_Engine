#pragma once

#include "Engine/Assets/ImportedMaterialData.h"
#include "Engine/Assets/ImportedMeshData.h"

#include <string>
#include <vector>

/**
 * @brief Часть импортированной модели, использующая один материал.
 */
struct ImportedMeshPart {
	ImportedMeshData mesh;
	std::string material_name;
};

/**
 * @brief Полный результат импорта модели.
 *
 * Содержит геометрию модели и материалы,
 * прочитанные из связанных MTL-файлов.
 */
struct ImportedModelData {
	std::vector<ImportedMeshPart> meshes;
	std::vector<ImportedMaterialData> materials;
};