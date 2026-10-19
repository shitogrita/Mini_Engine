#pragma once

#include "Engine/Assets/ImportedMaterialData.h"

#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief Выполняет разбор файлов материалов формата Wavefront MTL.
 *
 * MtlParser является частью системы импорта ресурсов Mini Engine
 * и отвечает за преобразование текстового MTL-файла
 * в набор структур ImportedMaterialData.
 *
 * MTL-файл обычно подключается из OBJ через директиву mtllib
 * и содержит описание одного или нескольких материалов.
 *
 * Для каждого материала MtlParser сохраняет параметры
 * в промежуточном представлении ImportedMaterialData.
 *
 * Поддерживаются следующие директивы MTL:
 *
 * - newmtl — имя материала;
 * - Ka — ambient color;
 * - Kd — diffuse color;
 * - Ks — specular color;
 * - Tf — transmission filter;
 * - Ns — shininess;
 * - Ni — optical density;
 * - illum — illumination model;
 * - map_Kd — diffuse texture.
 *
 * Путь, указанный в map_Kd, преобразуется
 * в путь файловой системы относительно директории MTL-файла,
 * если исходное значение является относительным.
 *
 * Парсер также обрабатывает дополнительные параметры map_Kd
 * и пути к изображениям, содержащие пробелы.
 *
 * MtlParser не создаёт runtime Material и Texture2D.
 * Его задача ограничивается чтением MTL
 * и формированием ImportedMaterialData.
 *
 * Преобразование ImportedMaterialData в Material
 * выполняется ModelImporter.
 */
class MtlParser {
public:
    /**
     * @brief Загружает и разбирает библиотеку материалов Wavefront MTL.
     *
     * Метод открывает указанный файл, последовательно
     * обрабатывает его строки и создаёт отдельный
     * ImportedMaterialData для каждой директивы newmtl.
     *
     * Перед началом разбора переданный контейнер materials
     * очищается.
     *
     * Если файл не удалось открыть либо в нём
     * не найдено ни одного корректного материала,
     * метод возвращает false.
     *
     * @param path Путь к MTL-файлу.
     * @param materials Контейнер, в который записываются
     * разобранные материалы.
     *
     * @return true, если был успешно прочитан
     * хотя бы один материал; иначе false.
     */
    static bool Parse(const std::filesystem::path& path, std::vector<ImportedMaterialData>& materials);

    /**
     * @brief Загружает MTL-файл по пути,
     * представленному строкой std::string.
     *
     * Перегрузка преобразует строковый путь
     * в std::filesystem::path и использует основной Parse().
     *
     * Поведение разбора полностью совпадает
     * с перегрузкой, принимающей std::filesystem::path.
     *
     * @param path Строковое представление пути к MTL-файлу.
     * @param materials Контейнер, в который записываются
     * разобранные материалы.
     *
     * @return true, если был успешно прочитан
     * хотя бы один материал; иначе false.
     */
    static bool Parse(const std::string& path, std::vector<ImportedMaterialData>& materials);
};