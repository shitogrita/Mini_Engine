#pragma once

#include "Engine/Math/matrix_types.h"

#include <filesystem>
#include <string>

/**
 * @brief Промежуточное представление материала,
 * загруженного из Wavefront MTL.
 *
 * Структура не является runtime-материалом Renderer.
 *
 * Её задача:
 *
 * MTL
 * -> ImportedMaterialData
 * -> Material
 *
 * Здесь хранятся исходные параметры MTL максимально
 * близко к их представлению в файле.
 */
struct ImportedMaterialData {
	/**
	 * @brief Имя материала из директивы newmtl.
	 *
	 * Это имя используется OBJ-файлом через usemtl
	 * для привязки частей геометрии к материалу.
	 */
	std::string name;

	/**
	 * @brief Ambient color из параметра Ka.
	 */
	Vec3 ambient_color{0.0f, 0.0f, 0.0f};

	/**
	 * @brief Diffuse color из параметра Kd.
	 */
	Vec3 diffuse_color{1.0f, 1.0f, 1.0f};

	/**
	 * @brief Specular color из параметра Ks.
	 */
	Vec3 specular_color{0.0f, 0.0f, 0.0f};

	/**
	 * @brief Transmission filter из параметра Tf.
	 *
	 * Пока Renderer не использует это значение,
	 * но оно сохраняется для будущей поддержки
	 * прозрачных и стеклянных материалов.
	 */
	Vec3 transmission_filter{1.0f, 1.0f, 1.0f};

	/**
	 * @brief Коэффициент блеска из параметра Ns.
	 */
	float shininess = 32.0f;

	/**
	 * @brief Optical density из параметра Ni.
	 *
	 * Значение потребуется для будущей реализации
	 * преломления прозрачных материалов.
	 */
	float optical_density = 1.0f;

	/**
	 * @brief Модель освещения из параметра illum.
	 */
	int illumination_model = 0;

	/**
	 * @brief Полный путь к diffuse-текстуре map_Kd.
	 *
	 * MtlParser преобразует относительный путь
	 * из MTL в путь относительно директории MTL.
	 *
	 * Например:
	 *
	 * /models/vase/vase.mtl
	 * map_Kd Chrysanthemum.jpg
	 *
	 * превращается в:
	 *
	 * /models/vase/Chrysanthemum.jpg
	 */
	std::filesystem::path diffuse_texture_path;
};