#include "Engine/Assets/MtlParser.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Удаляет пробельные символы
 * в начале и конце строки.
 *
 * Обрабатываются обычные пробелы, табуляция,
 * символы возврата каретки и перевода строки.
 *
 * Функция используется при разборе строк MTL,
 * чтобы команды и их значения не зависели
 * от форматирования исходного файла.
 *
 * @param value Исходная строка.
 *
 * @return Строка без внешних пробельных символов.
 * Если строка содержит только пробелы,
 * возвращается пустая строка.
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
 * @brief Разбирает три вещественных значения MTL.
 *
 * Функция используется для параметров,
 * представленных тремя компонентами:
 *
 * - Ka;
 * - Kd;
 * - Ks;
 * - Tf.
 *
 * Значения последовательно записываются
 * в компоненты x, y и z результирующего Vec3.
 *
 * @param value Текстовая часть строки после команды MTL.
 * @param result Vec3, в который записываются прочитанные значения.
 *
 * @return true, если удалось прочитать все три компонента;
 * иначе false.
 */
static bool ParseMtlVec3(const std::string& value, Vec3& result) {
    std::istringstream stream(value);

    return static_cast<bool>(
        stream >> result.x >> result.y >> result.z
    );
}

/**
 * @brief Разбирает одно вещественное значение MTL.
 *
 * Используется для скалярных параметров,
 * таких как Ns и Ni.
 *
 * @param value Текстовое представление значения.
 * @param result Переменная, в которую записывается результат.
 *
 * @return true при успешном чтении значения;
 * иначе false.
 */
static bool ParseMtlFloat(const std::string& value, float& result) {
    std::istringstream stream(value);

    return static_cast<bool>(
        stream >> result
    );
}

/**
 * @brief Разбирает одно целочисленное значение MTL.
 *
 * Используется для параметра illum,
 * определяющего номер модели освещения MTL.
 *
 * @param value Текстовое представление значения.
 * @param result Переменная, в которую записывается результат.
 *
 * @return true при успешном чтении значения;
 * иначе false.
 */
static bool ParseMtlInt(const std::string& value, int& result) {
    std::istringstream stream(value);

    return static_cast<bool>(
        stream >> result
    );
}

/**
 * @brief Проверяет, является ли строковый токен числом.
 *
 * Проверка используется при обработке дополнительных
 * параметров директивы map_Kd.
 *
 * Например:
 *
 * map_Kd -o 0 0 0 texture.jpg
 *
 * Значения после параметра -o являются числами
 * и должны быть пропущены перед извлечением
 * фактического пути к изображению.
 *
 * Для проверки используется std::strtof().
 *
 * @param value Проверяемый токен.
 *
 * @return true, если вся строка представляет число;
 * иначе false.
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
 * @brief Удаляет парные внешние кавычки из строки.
 *
 * Функция поддерживает как двойные,
 * так и одинарные кавычки.
 *
 * Например:
 *
 * "texture name.jpg"
 *
 * преобразуется в:
 *
 * texture name.jpg
 *
 * Аналогично обрабатывается:
 *
 * 'texture name.jpg'
 *
 * @param value Исходная строка.
 *
 * @return Строка без окружающих кавычек.
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
 * @brief Извлекает путь к diffuse texture
 * из содержимого директивы map_Kd.
 *
 * Wavefront MTL позволяет указывать перед путём
 * дополнительные параметры текстуры.
 *
 * Например:
 *
 * map_Kd -s 1 1 1 texture.jpg
 *
 * map_Kd -o 0 0 0 texture.jpg
 *
 * map_Kd -clamp on texture.jpg
 *
 * Функция пропускает поддерживаемые дополнительные параметры
 * и возвращает непосредственно путь к изображению.
 *
 * Обрабатываются параметры:
 *
 * - -mm;
 * - -o;
 * - -s;
 * - -t;
 * - -blendu;
 * - -blendv;
 * - -boost;
 * - -texres;
 * - -clamp;
 * - -bm;
 * - -imfchan;
 * - -type.
 *
 * Если путь содержит пробелы,
 * оставшиеся токены объединяются обратно
 * в одну строку.
 *
 * Окружающие одинарные или двойные кавычки
 * удаляются перед возвратом результата.
 *
 * @param value Содержимое строки после команды map_Kd.
 *
 * @return Путь к изображению в форме,
 * в которой он был указан в MTL.
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

    /**
     * Путь к Texture может содержать пробелы.
     *
     * После обработки параметров map_Kd
     * все оставшиеся токены относятся к пути,
     * поэтому они объединяются обратно
     * в единую строку.
     */
    for (++index; index < tokens.size(); ++index) {
        texture_path += " ";
        texture_path += tokens[index];
    }

    return RemoveMtlQuotes(texture_path);
}

/**
 * @brief Преобразует путь из map_Kd
 * в нормализованный путь файловой системы.
 *
 * Если texture_path является абсолютным,
 * он используется непосредственно.
 *
 * Если путь относительный, он разрешается
 * относительно директории текущего MTL-файла.
 *
 * Например:
 *
 * /models/vase/vase.mtl
 *
 * и:
 *
 * map_Kd Chrysanthemum.jpg
 *
 * дают:
 *
 * /models/vase/Chrysanthemum.jpg
 *
 * Дополнительно обратные слеши,
 * встречающиеся в MTL-файлах, созданных на Windows,
 * преобразуются в прямые слеши.
 *
 * После формирования выполняется
 * лексическая нормализация пути.
 *
 * @param mtl_path Путь к текущему MTL-файлу.
 * @param texture_path Путь к изображению из map_Kd.
 *
 * @return Нормализованный путь к diffuse texture.
 */
static std::filesystem::path ResolveDiffuseTexturePath(const std::filesystem::path& mtl_path, std::string texture_path) {
    if (texture_path.empty()) {
        return {};
    }

    /**
     * MTL-файл может содержать Windows-разделители пути.
     *
     * Преобразование делает путь пригодным
     * для дальнейшей обработки std::filesystem
     * на текущей платформе.
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
 * @brief Загружает MTL-файл по пути,
 * заданному строкой std::string.
 *
 * Перегрузка преобразует строку
 * в std::filesystem::path и передаёт работу
 * основной реализации Parse().
 *
 * @param path Путь к MTL-файлу.
 * @param materials Контейнер для разобранных материалов.
 *
 * @return true, если был успешно прочитан
 * хотя бы один материал; иначе false.
 */
bool MtlParser::Parse(const std::string& path, std::vector<ImportedMaterialData>& materials) {
    return Parse(
        std::filesystem::path(path),
        materials
    );
}

/**
 * @brief Загружает и разбирает библиотеку материалов Wavefront MTL.
 *
 * Метод читает файл построчно.
 *
 * Для каждой строки:
 *
 * 1. удаляется комментарий после символа #;
 * 2. удаляются внешние пробелы;
 * 3. выделяются команда и значение;
 * 4. поддерживаемая команда записывается
 *    в текущий ImportedMaterialData.
 *
 * Каждая директива newmtl начинает описание
 * нового материала.
 *
 * Перед переходом к следующему newmtl
 * предыдущий материал сохраняется
 * в результирующий контейнер.
 *
 * Поддерживается следующее соответствие:
 *
 * - Ka -> ambient_color;
 * - Kd -> diffuse_color;
 * - Ks -> specular_color;
 * - Tf -> transmission_filter;
 * - Ns -> shininess;
 * - Ni -> optical_density;
 * - illum -> illumination_model;
 * - map_Kd -> diffuse_texture_path.
 *
 * Строки до первого newmtl игнорируются.
 *
 * @param path Путь к MTL-файлу.
 * @param materials Контейнер для разобранных материалов.
 *
 * @return true, если после разбора получен
 * хотя бы один материал; иначе false.
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
     * Сохраняет текущий полностью сформированный материал.
     *
     * Лямбда вызывается:
     *
     * - перед началом следующего newmtl;
     * - после завершения чтения файла.
     *
     * Материал без имени не добавляется
     * в результирующий список.
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
        /**
         * Комментарии MTL начинаются с символа #.
         *
         * Всё содержимое после него
         * не участвует в разборе строки.
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

        /**
         * Строка разделяется на команду
         * и оставшуюся часть значения.
         *
         * Например:
         *
         * Kd 1.0 0.0 0.0
         *
         * превращается в:
         *
         * command = "Kd"
         * value   = "1.0 0.0 0.0"
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

        /**
         * newmtl начинает описание нового материала.
         *
         * Сначала сохраняется предыдущий материал,
         * после чего структура сбрасывается
         * к значениям по умолчанию.
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

        /**
         * Параметры, расположенные до первого newmtl,
         * не относятся ни к одному материалу
         * и поэтому игнорируются.
         */
        if (!has_material) {
            continue;
        }

        /**
         * Ambient color.
         */
        if (command == "Ka") {
            ParseMtlVec3(
                value,
                current_material.ambient_color
            );

            continue;
        }

        /**
         * Diffuse color.
         */
        if (command == "Kd") {
            ParseMtlVec3(
                value,
                current_material.diffuse_color
            );

            continue;
        }

        /**
         * Specular color.
         */
        if (command == "Ks") {
            ParseMtlVec3(
                value,
                current_material.specular_color
            );

            continue;
        }

        /**
         * Transmission filter.
         */
        if (command == "Tf") {
            ParseMtlVec3(
                value,
                current_material.transmission_filter
            );

            continue;
        }

        /**
         * Shininess.
         */
        if (command == "Ns") {
            ParseMtlFloat(
                value,
                current_material.shininess
            );

            continue;
        }

        /**
         * Optical density.
         */
        if (command == "Ni") {
            ParseMtlFloat(
                value,
                current_material.optical_density
            );

            continue;
        }

        /**
         * Illumination model.
         */
        if (command == "illum") {
            ParseMtlInt(
                value,
                current_material.illumination_model
            );

            continue;
        }

        /**
         * Diffuse texture.
         *
         * Сначала из map_Kd извлекается путь
         * без дополнительных параметров,
         * после чего он разрешается относительно
         * директории текущего MTL-файла.
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

    /**
     * После достижения конца файла последний материал
     * ещё не был сохранён вызовом следующего newmtl,
     * поэтому сохраняем его отдельно.
     */
    store_material();

    return !materials.empty();
}