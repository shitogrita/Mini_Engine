#pragma once

#include "Engine/Math/matrix_types.h"

#include <filesystem>
#include <string>

/**
 * @brief Промежуточное представление материала,
 * прочитанного из Wavefront MTL.
 *
 * ImportedMaterialData хранит параметры материала
 * в форме, близкой к исходным директивам MTL-файла.
 *
 * Структура используется на этапе импорта модели
 * и не является runtime-материалом Renderer.
 *
 * Общая последовательность преобразования:
 *
 * MTL
 * -> ImportedMaterialData
 * -> Material.
 *
 * MtlParser заполняет поля ImportedMaterialData,
 * после чего ModelImporter преобразует поддерживаемые
 * параметры в Material, используемый при рендеринге сцены.
 *
 * В структуре также сохраняются параметры MTL,
 * которые в текущем Renderer непосредственно
 * не участвуют в вычислении освещения.
 *
 * ImportedMaterialData не владеет OpenGL-ресурсами
 * и содержит только данные в памяти центрального процессора.
 */
struct ImportedMaterialData {
    /**
     * @brief Имя материала из директивы newmtl.
     *
     * Имя является идентификатором материала внутри MTL-файла.
     *
     * OBJ связывает часть геометрии с этим материалом
     * через директиву usemtl.
     *
     * ModelImporter использует это значение,
     * чтобы сопоставить ImportedMeshPart
     * с соответствующим ImportedMaterialData.
     */
    std::string name;

    /**
     * @brief Ambient color из параметра Ka.
     *
     * Содержит RGB-компоненты фонового отражения материала.
     *
     * При создании runtime Material текущее значение
     * преобразуется в скалярный AmbientStrength.
     */
    Vec3 ambient_color{0.0f, 0.0f, 0.0f};

    /**
     * @brief Diffuse color из параметра Kd.
     *
     * Определяет основной цвет поверхности материала.
     *
     * Если diffuse texture отсутствует,
     * ModelImporter использует это значение
     * как базовый Color runtime Material.
     *
     * Если задана map_Kd, runtime Material
     * использует текстуру как основной источник цвета.
     */
    Vec3 diffuse_color{1.0f, 1.0f, 1.0f};

    /**
     * @brief Specular color из параметра Ks.
     *
     * Содержит RGB-компоненты зеркального отражения материала.
     *
     * Текущий runtime Material использует
     * скалярный SpecularStrength, поэтому ModelImporter
     * преобразует RGB-значение в одну интенсивность.
     */
    Vec3 specular_color{0.0f, 0.0f, 0.0f};

    /**
     * @brief Transmission filter из параметра Tf.
     *
     * Хранит цветовой коэффициент прохождения света
     * через материал в формате MTL.
     *
     * Значение сохраняется при разборе материала,
     * но текущий Renderer непосредственно
     * не использует его при отрисовке.
     */
    Vec3 transmission_filter{1.0f, 1.0f, 1.0f};

    /**
     * @brief Коэффициент блеска из параметра Ns.
     *
     * Определяет характеристику specular highlight
     * материала.
     *
     * ModelImporter передаёт это значение
     * в Material::Shininess.
     */
    float shininess = 32.0f;

    /**
     * @brief Optical density из параметра Ni.
     *
     * Представляет оптическую плотность материала,
     * указанную в MTL-файле.
     *
     * Значение сохраняется в промежуточной структуре,
     * но текущий Renderer непосредственно
     * не использует его при отрисовке.
     */
    float optical_density = 1.0f;

    /**
     * @brief Модель освещения из параметра illum.
     *
     * Значение соответствует номеру illumination model,
     * указанному в MTL.
     *
     * Оно сохраняется при импорте,
     * но текущий Renderer не выбирает алгоритм освещения
     * на основании этого параметра.
     */
    int illumination_model = 0;

    /**
     * @brief Полный путь к diffuse texture из параметра map_Kd.
     *
     * MtlParser разрешает путь относительно директории,
     * в которой расположен MTL-файл.
     *
     * Например:
     *
     * MTL:
     *
     * /models/vase/vase.mtl
     *
     * содержит:
     *
     * map_Kd Chrysanthemum.jpg
     *
     * В ImportedMaterialData сохраняется путь:
     *
     * /models/vase/Chrysanthemum.jpg
     *
     * ModelImporter передаёт этот путь в TextureManager,
     * который загружает соответствующий Texture2D.
     *
     * Путь также сохраняется в runtime Material
     * для отображения в Inspector и сериализации Scene.
     */
    std::filesystem::path diffuse_texture_path;
};