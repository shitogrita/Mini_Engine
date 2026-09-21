#pragma once

#include "ImportedMaterialData.h"
#include "ImportedMeshData.h"

#include <string>
#include <vector>

/**
 * @brief Часть импортированной модели, использующая один материал.
 *
 * Одна OBJ-модель может содержать несколько материалов.
 * Поэтому геометрия разделяется на части по используемому материалу.
 */
struct ImportedMeshPart {
	ImportedMeshData mesh;
	std::string material_name;
};

/**
 * @brief Полный результат импорта OBJ-модели.
 *
 * Содержит геометрические части модели и материалы,
 * прочитанные из связанного MTL-файла.
 */
struct ImportedModelData {
	std::vector<ImportedMeshPart> meshes;
	std::vector<ImportedMaterialData> materials;
};