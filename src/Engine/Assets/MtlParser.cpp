#include "Engine/Assets/MtlParser.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Удаляет пробельные символы в начале и конце строки.
 *
 * @param value Исходная строка.
 *
 * @return Строка без внешних пробелов.
 */
static std::string TrimMtlText(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

/**
 * @brief Разбирает три float-значения MTL.
 *
 * Используется для Ka, Kd, Ks и Tf.
 *
 * @param value Текст после команды MTL.
 * @param result Результирующий Vec3.
 *
 * @return true при успешном чтении трёх чисел.
 */
static bool ParseMtlVec3(const std::string& value, Vec3& result) {
    std::istringstream stream(value);

    return static_cast<bool>(
        stream >> result.x >> result.y >> result.z
    );
}

/**
 * @brief Разбирает одно float-значение.
 *
 * @param value Строковое значение.
 * @param result Результат.
 *
 * @return true при успешном преобразовании.
 */
static bool ParseMtlFloat(const std::string& value, float& result) {
    std::istringstream stream(value);

    return static_cast<bool>(
        stream >> result
    );
}

/**
 * @brief Разбирает одно целочисленное значение.
 *
 * Используется для параметра illum.
 *
 * @param value Строковое значение.
 * @param result Результат.
 *
 * @return true при успешном преобразовании.
 */
static bool ParseMtlInt(const std::string& value, int& result) {
    std::istringstream stream(value);

    return static_cast<bool>(
        stream >> result
    );
}

/**
 * @brief Проверяет, является ли токен числом.
 *
 * Используется при пропуске параметров map_Kd,
 * например:
 *
 * map_Kd -o 0 0 0 texture.jpg
 *
 * @param value Проверяемая строка.
 *
 * @return true, если вся строка является числом.
 */
static bool IsMtlNumber(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;

    std::strtof(
        value.c_str(),
        &end
    );

    return end != value.c_str() && *end == '\0';
}

/**
 * @brief Удаляет окружающие кавычки из пути.
 *
 * Поддерживаются:
 *
 * "texture name.jpg"
 *
 * и
 *
 * 'texture name.jpg'
 *
 * @param value Исходная строка.
 *
 * @return Путь без внешних кавычек.
 */
static std::string RemoveMtlQuotes(std::string value) {
    value = TrimMtlText(value);

    if (value.size() < 2) {
        return value;
    }

    const bool double_quotes =
        value.front() == '"' &&
        value.back() == '"';

    const bool single_quotes =
        value.front() == '\'' &&
        value.back() == '\'';

    if (double_quotes || single_quotes) {
        return value.substr(
            1,
            value.size() - 2
        );
    }

    return value;
}

/**
 * @brief Извлекает имя файла текстуры из map_Kd.
 *
 * Wavefront допускает дополнительные параметры:
 *
 * map_Kd -s 1 1 1 texture.jpg
 * map_Kd -o 0 0 0 texture.jpg
 * map_Kd -clamp on texture.jpg
 *
 * Функция пропускает известные параметры
 * и возвращает непосредственно путь к изображению.
 *
 * @param value Всё содержимое строки после map_Kd.
 *
 * @return Путь, записанный в MTL.
 */
static std::string ExtractDiffuseTexturePath(const std::string& value) {
    std::istringstream stream(value);

    std::vector<std::string> tokens;
    std::string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return {};
    }

    std::size_t index = 0;

    while (
        index < tokens.size() &&
        !tokens[index].empty() &&
        tokens[index][0] == '-'
    ) {
        const std::string option = tokens[index++];

        if (option == "-mm") {
            index = std::min(
                index + 2,
                tokens.size()
            );

            continue;
        }

        if (
            option == "-o" ||
            option == "-s" ||
            option == "-t"
        ) {
            int value_count = 0;

            while (
                index < tokens.size() &&
                value_count < 3 &&
                IsMtlNumber(tokens[index])
            ) {
                ++index;
                ++value_count;
            }

            continue;
        }

        if (
            option == "-blendu" ||
            option == "-blendv" ||
            option == "-boost" ||
            option == "-texres" ||
            option == "-clamp" ||
            option == "-bm" ||
            option == "-imfchan" ||
            option == "-type"
        ) {
            if (index < tokens.size()) {
                ++index;
            }

            continue;
        }

        break;
    }

    if (index >= tokens.size()) {
        return {};
    }

    std::string texture_path = tokens[index];

    /*
     * Путь может содержать пробелы.
     *
     * Поэтому все оставшиеся токены снова
     * объединяются в одну строку.
     */
    for (++index; index < tokens.size(); ++index) {
        texture_path += " ";
        texture_path += tokens[index];
    }

    return RemoveMtlQuotes(texture_path);
}

/**
 * @brief Преобразует путь map_Kd в путь файловой системы.
 *
 * Если путь абсолютный, он используется напрямую.
 *
 * Если путь относительный, он разрешается относительно
 * директории MTL-файла.
 *
 * Например:
 *
 * /models/vase/vase.mtl
 * map_Kd Chrysanthemum.jpg
 *
 * даст:
 *
 * /models/vase/Chrysanthemum.jpg
 *
 * @param mtl_path Путь к текущему MTL.
 * @param texture_path Путь из map_Kd.
 *
 * @return Нормализованный путь к текстуре.
 */
static std::filesystem::path ResolveDiffuseTexturePath(const std::filesystem::path& mtl_path, std::string texture_path) {
    if (texture_path.empty()) {
        return {};
    }

    /*
     * Некоторые MTL были созданы на Windows
     * и содержат обратные слеши.
     *
     * Приводим их к переносимому виду.
     */
    std::replace(
        texture_path.begin(),
        texture_path.end(),
        '\\',
        '/'
    );

    std::filesystem::path resolved_path(texture_path);

    if (resolved_path.is_relative()) {
        resolved_path =
            mtl_path.parent_path() /
            resolved_path;
    }

    return resolved_path.lexically_normal();
}

/**
 * @brief Загружает материалы по пути, заданному строкой.
 *
 * @param path Путь к MTL.
 * @param materials Результирующий список материалов.
 *
 * @return true при успешной загрузке хотя бы одного материала.
 */
bool MtlParser::Parse(const std::string& path, std::vector<ImportedMaterialData>& materials) {
    return Parse(
        std::filesystem::path(path),
        materials
    );
}

/**
 * @brief Загружает библиотеку материалов Wavefront MTL.
 *
 * Каждый newmtl создаёт отдельный ImportedMaterialData.
 *
 * Поддерживаемые параметры:
 *
 * Ka     -> ambient_color;
 * Kd     -> diffuse_color;
 * Ks     -> specular_color;
 * Tf     -> transmission_filter;
 * Ns     -> shininess;
 * Ni     -> optical_density;
 * illum  -> illumination_model;
 * map_Kd -> diffuse_texture_path.
 *
 * @param path Путь к MTL-файлу.
 * @param materials Результирующий список материалов.
 *
 * @return true, если был найден хотя бы один newmtl.
 */
bool MtlParser::Parse(const std::filesystem::path& path, std::vector<ImportedMaterialData>& materials) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return false;
    }

    materials.clear();

    ImportedMaterialData current_material;
    bool has_material = false;

    /**
     * Сохраняет законченный материал перед
     * началом следующего newmtl или концом файла.
     */
    const auto store_material = [&]() {
        if (
            !has_material ||
            current_material.name.empty()
        ) {
            return;
        }

        materials.push_back(
            current_material
        );
    };

    std::string line;

    while (std::getline(file, line)) {
        /*
         * Удаляем комментарий.
         */
        const std::size_t comment_position =
            line.find('#');

        if (comment_position != std::string::npos) {
            line.erase(
                comment_position
            );
        }

        line = TrimMtlText(line);

        if (line.empty()) {
            continue;
        }

        /*
         * Разделяем строку на:
         *
         * command value
         *
         * Например:
         *
         * Kd 1.0 0.0 0.0
         */
        const std::size_t separator =
            line.find_first_of(" \t");

        const std::string command =
            separator == std::string::npos
                ? line
                : line.substr(0, separator);

        const std::string value =
            separator == std::string::npos
                ? std::string{}
                : TrimMtlText(
                    line.substr(separator + 1)
                );

        /*
         * Начало нового материала.
         */
        if (command == "newmtl") {
            store_material();

            current_material =
                ImportedMaterialData{};

            current_material.name =
                value;

            has_material = true;

            continue;
        }

        /*
         * Параметры до первого newmtl
         * игнорируются.
         */
        if (!has_material) {
            continue;
        }

        /*
         * Ambient color.
         */
        if (command == "Ka") {
            ParseMtlVec3(
                value,
                current_material.ambient_color
            );

            continue;
        }

        /*
         * Diffuse color.
         */
        if (command == "Kd") {
            ParseMtlVec3(
                value,
                current_material.diffuse_color
            );

            continue;
        }

        /*
         * Specular color.
         */
        if (command == "Ks") {
            ParseMtlVec3(
                value,
                current_material.specular_color
            );

            continue;
        }

        /*
         * Transmission filter.
         */
        if (command == "Tf") {
            ParseMtlVec3(
                value,
                current_material.transmission_filter
            );

            continue;
        }

        /*
         * Shininess.
         */
        if (command == "Ns") {
            ParseMtlFloat(
                value,
                current_material.shininess
            );

            continue;
        }

        /*
         * Optical density.
         */
        if (command == "Ni") {
            ParseMtlFloat(
                value,
                current_material.optical_density
            );

            continue;
        }

        /*
         * Illumination model.
         */
        if (command == "illum") {
            ParseMtlInt(
                value,
                current_material.illumination_model
            );

            continue;
        }

        /*
         * Diffuse texture.
         */
        if (command == "map_Kd") {
            const std::string texture_path =
                ExtractDiffuseTexturePath(value);

            current_material.diffuse_texture_path =
                ResolveDiffuseTexturePath(
                    path,
                    texture_path
                );

            continue;
        }
    }

    /*
     * Сохраняем последний материал.
     */
    store_material();

    return !materials.empty();
}