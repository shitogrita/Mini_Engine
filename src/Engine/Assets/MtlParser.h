#pragma once

#include "Engine/Assets/ImportedMaterialData.h"

#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief Парсер Wavefront MTL.
 *
 * MtlParser читает библиотеку материалов,
 * связанную с OBJ-файлом.
 *
 * Поддерживаемые параметры:
 *
 * newmtl - имя материала;
 * Ka     - ambient color;
 * Kd     - diffuse color;
 * Ks     - specular color;
 * Tf     - transmission filter;
 * Ns     - shininess;
 * Ni     - optical density;
 * illum  - illumination model;
 * map_Kd - diffuse texture.
 *
 * Пути map_Kd разрешаются относительно директории
 * самого MTL-файла.
 */
class MtlParser {
public:
	/**
	 * @brief Загружает материалы из MTL-файла.
	 *
	 * @param path Путь к MTL-файлу.
	 * @param materials Контейнер для загруженных материалов.
	 *
	 * @return true, если был успешно прочитан хотя бы один материал.
	 */
	static bool Parse(const std::filesystem::path& path, std::vector<ImportedMaterialData>& materials);

	/**
	 * @brief Перегрузка Parse для std::string.
	 *
	 * @param path Путь к MTL-файлу.
	 * @param materials Контейнер для загруженных материалов.
	 *
	 * @return true, если был успешно прочитан хотя бы один материал.
	 */
	static bool Parse(const std::string& path, std::vector<ImportedMaterialData>& materials);
};