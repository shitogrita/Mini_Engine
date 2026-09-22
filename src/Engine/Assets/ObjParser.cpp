#include "Engine/Assets/ObjParser.h"

#include "Engine/Assets/MtlParser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @brief Индексы одной вершины полигона OBJ.
 *
 * Формат Wavefront OBJ позволяет задавать вершину
 * несколькими способами:
 *
 * v
 * v/vt
 * v//vn
 * v/vt/vn
 *
 * Поэтому position присутствует всегда,
 * а tex_coord и normal являются необязательными.
 */
struct ObjVertexIndices {
    int position = 0;
    int tex_coord = 0;
    int normal = 0;

    bool has_tex_coord = false;
    bool has_normal = false;
};

/**
 * @brief Ключ уникальной render-вершины.
 *
 * В OBJ одна position может использоваться
 * с разными UV или normal.
 *
 * Поэтому нельзя использовать только индекс position
 * как индекс GPU-вершины.
 *
 * Уникальная render-вершина определяется комбинацией:
 *
 * position + tex_coord + normal.
 */
struct RenderVertexKey {
    std::uint32_t position = 0;
    std::uint32_t tex_coord = 0;
    std::uint32_t normal = 0;

    bool has_tex_coord = false;
    bool has_normal = false;

    /**
     * @brief Сравнивает два ключа render-вершин.
     */
    bool operator==(const RenderVertexKey& other) const {
        return position == other.position &&
               tex_coord == other.tex_coord &&
               normal == other.normal &&
               has_tex_coord == other.has_tex_coord &&
               has_normal == other.has_normal;
    }
};

/**
 * @brief Hash-функция для RenderVertexKey.
 *
 * Используется unordered_map для быстрого поиска
 * уже созданных GPU-вершин.
 */
struct RenderVertexKeyHash {
    std::size_t operator()(const RenderVertexKey& key) const {
        std::size_t hash = std::hash<std::uint32_t>{}(key.position);

        hash ^= std::hash<std::uint32_t>{}(key.tex_coord) << 1;
        hash ^= std::hash<std::uint32_t>{}(key.normal) << 2;
        hash ^= std::hash<bool>{}(key.has_tex_coord) << 3;
        hash ^= std::hash<bool>{}(key.has_normal) << 4;

        return hash;
    }
};

/**
 * @brief Кэш render-вершин.
 *
 * Ключ:
 *
 * position / tex_coord / normal
 *
 * Значение:
 *
 * индекс Vertex в render_vertices.
 */
using RenderVertexCache = std::unordered_map<RenderVertexKey, std::uint32_t, RenderVertexKeyHash>;

/**
 * @brief Проверяет, является ли символ пробельным.
 *
 * @param symbol Проверяемый символ.
 *
 * @return true для пробельного символа.
 */
static bool IsSpace(char symbol) {
    return std::isspace(
        static_cast<unsigned char>(symbol)
    ) != 0;
}

/**
 * @brief Пропускает пробельные символы.
 *
 * @param current Текущая позиция в строке.
 * @param end Конец строки.
 */
static void SkipSpaces(const char*& current, const char* end) {
    while (
        current < end &&
        IsSpace(*current)
    ) {
        ++current;
    }
}

/**
 * @brief Удаляет пробелы в начале и конце строки.
 *
 * Используется для строковых директив OBJ:
 *
 * mtllib
 * usemtl
 *
 * @param value Исходная строка.
 *
 * @return Строка без внешних пробелов.
 */
static std::string TrimObjText(const std::string& value) {
    const std::size_t first =
        value.find_first_not_of(
            " \t\r\n"
        );

    if (first == std::string::npos) {
        return {};
    }

    const std::size_t last =
        value.find_last_not_of(
            " \t\r\n"
        );

    return value.substr(
        first,
        last - first + 1
    );
}

/**
 * @brief Удаляет внешние кавычки из строки.
 *
 * Это позволяет обрабатывать пути вида:
 *
 * mtllib "My Materials.mtl"
 *
 * @param value Исходная строка.
 *
 * @return Строка без внешних кавычек.
 */
static std::string RemoveObjQuotes(std::string value) {
    value = TrimObjText(value);

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
 * @brief Читает float из текущей позиции строки.
 *
 * Используется std::from_chars,
 * чтобы не создавать stringstream для каждой вершины.
 *
 * @param current Текущая позиция.
 * @param end Конец строки.
 * @param value Результат.
 *
 * @return true при успешном чтении.
 */
static bool ParseFloat(const char*& current, const char* end, float& value) {
    SkipSpaces(
        current,
        end
    );

    if (current >= end) {
        return false;
    }

    const auto [next, error] =
        std::from_chars(
            current,
            end,
            value
        );

    if (error != std::errc{}) {
        return false;
    }

    current = next;

    return true;
}

/**
 * @brief Читает integer из текущей позиции строки.
 *
 * @param current Текущая позиция.
 * @param end Конец строки.
 * @param value Результат.
 *
 * @return true при успешном чтении.
 */
static bool ParseInteger(const char*& current, const char* end, int& value) {
    SkipSpaces(
        current,
        end
    );

    if (current >= end) {
        return false;
    }

    const auto [next, error] =
        std::from_chars(
            current,
            end,
            value
        );

    if (error != std::errc{}) {
        return false;
    }

    current = next;

    return true;
}

/**
 * @brief Разбирает строку position.
 *
 * Ожидаемый формат:
 *
 * v x y z
 *
 * @param line Строка OBJ.
 * @param position Полученная позиция.
 *
 * @return true при успешном разборе.
 */
static bool ParsePositionLine(std::string_view line, Vec3& position) {
    if (
        line.size() < 2 ||
        line[0] != 'v' ||
        !IsSpace(line[1])
    ) {
        return false;
    }

    const char* current =
        line.data() + 1;

    const char* end =
        line.data() +
        line.size();

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (!ParseFloat(current, end, x)) {
        return false;
    }

    if (!ParseFloat(current, end, y)) {
        return false;
    }

    if (!ParseFloat(current, end, z)) {
        return false;
    }

    position =
        Vec3{
            x,
            y,
            z
        };

    return true;
}

/**
 * @brief Разбирает texture coordinate.
 *
 * Ожидаемый формат:
 *
 * vt u v
 *
 * @param line Строка OBJ.
 * @param tex_coord Полученные UV.
 *
 * @return true при успешном разборе.
 */
static bool ParseTexCoordLine(std::string_view line, Vec2& tex_coord) {
    if (
        line.size() < 3 ||
        line[0] != 'v' ||
        line[1] != 't' ||
        !IsSpace(line[2])
    ) {
        return false;
    }

    const char* current =
        line.data() + 2;

    const char* end =
        line.data() +
        line.size();

    float u = 0.0f;
    float v = 0.0f;

    if (!ParseFloat(current, end, u)) {
        return false;
    }

    if (!ParseFloat(current, end, v)) {
        return false;
    }

    tex_coord =
        Vec2{
            u,
            v
        };

    return true;
}

/**
 * @brief Разбирает normal.
 *
 * Ожидаемый формат:
 *
 * vn x y z
 *
 * @param line Строка OBJ.
 * @param normal Полученная нормаль.
 *
 * @return true при успешном разборе.
 */
static bool ParseNormalLine(std::string_view line, Vec3& normal) {
    if (
        line.size() < 3 ||
        line[0] != 'v' ||
        line[1] != 'n' ||
        !IsSpace(line[2])
    ) {
        return false;
    }

    const char* current =
        line.data() + 2;

    const char* end =
        line.data() +
        line.size();

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (!ParseFloat(current, end, x)) {
        return false;
    }

    if (!ParseFloat(current, end, y)) {
        return false;
    }

    if (!ParseFloat(current, end, z)) {
        return false;
    }

    normal =
        Vec3{
            x,
            y,
            z
        };

    return true;
}

/**
 * @brief Преобразует индекс OBJ в индекс C++.
 *
 * OBJ использует индексацию начиная с 1:
 *
 * 1 -> первый элемент.
 *
 * Также поддерживаются отрицательные индексы:
 *
 * -1 -> последний прочитанный элемент.
 *
 * @param obj_index Индекс из OBJ.
 * @param elements_count Размер соответствующего массива.
 * @param result Результирующий zero-based индекс.
 *
 * @return true, если индекс корректен.
 */
static bool ConvertObjIndex(int obj_index, std::size_t elements_count, std::uint32_t& result) {
    int index = 0;

    if (obj_index > 0) {
        index =
            obj_index - 1;
    } else if (obj_index < 0) {
        index =
            static_cast<int>(
                elements_count
            ) +
            obj_index;
    } else {
        /*
         * Индекс 0 запрещён спецификацией OBJ.
         */
        return false;
    }

    if (
        index < 0 ||
        index >= static_cast<int>(elements_count)
    ) {
        return false;
    }

    result =
        static_cast<std::uint32_t>(
            index
        );

    return true;
}

/**
 * @brief Разбирает одну вершину полигона OBJ.
 *
 * Поддерживаются форматы:
 *
 * v
 * v/vt
 * v//vn
 * v/vt/vn
 *
 * Например:
 *
 * 5/2/8
 *
 * означает:
 *
 * position = 5
 * tex_coord = 2
 * normal = 8
 *
 * @param current Текущая позиция в строке.
 * @param end Конец строки.
 * @param indices Полученные индексы.
 *
 * @return true при успешном разборе.
 */
static bool ParseFaceVertexIndices(const char*& current, const char* end, ObjVertexIndices& indices) {
    SkipSpaces(
        current,
        end
    );

    if (!ParseInteger(
            current,
            end,
            indices.position
        )) {
        return false;
    }

    /*
     * Формат:
     *
     * v
     */
    if (
        current >= end ||
        IsSpace(*current)
    ) {
        return true;
    }

    if (*current != '/') {
        return false;
    }

    ++current;

    /*
     * Если следующий символ не '/',
     * читаем texture coordinate.
     *
     * Формат:
     *
     * v/vt
     *
     * или:
     *
     * v/vt/vn
     */
    if (
        current < end &&
        *current != '/'
    ) {
        if (!ParseInteger(
                current,
                end,
                indices.tex_coord
            )) {
            return false;
        }

        indices.has_tex_coord =
            true;
    }

    /*
     * Формат v/vt закончился.
     */
    if (
        current >= end ||
        IsSpace(*current)
    ) {
        return true;
    }

    if (*current != '/') {
        return false;
    }

    ++current;

    /*
     * После второго '/' должна находиться normal.
     */
    if (!ParseInteger(
            current,
            end,
            indices.normal
        )) {
        return false;
    }

    indices.has_normal =
        true;

    return true;
}

/**
 * @brief Получает существующую render-вершину
 * или создаёт новую.
 *
 * OBJ позволяет одной position иметь разные UV и normals.
 * Поэтому одна геометрическая вершина может превратиться
 * в несколько GPU-вершин.
 *
 * @param obj_vertex Индексы OBJ.
 * @param positions Все position OBJ.
 * @param tex_coords Все UV OBJ.
 * @param normals Все normals OBJ.
 * @param render_vertices GPU-вершины текущего Mesh.
 * @param vertex_cache Кэш уже созданных вершин.
 * @param render_index Индекс найденной или созданной вершины.
 *
 * @return true при корректных индексах.
 */
static bool GetOrCreateRenderVertex(const ObjVertexIndices& obj_vertex, const std::vector<Vec3>& positions, const std::vector<Vec2>& tex_coords, const std::vector<Vec3>& normals, std::vector<Vertex>& render_vertices, RenderVertexCache& vertex_cache, std::uint32_t& render_index) {
    RenderVertexKey key{};

    if (!ConvertObjIndex(
            obj_vertex.position,
            positions.size(),
            key.position
        )) {
        return false;
    }

    if (obj_vertex.has_tex_coord) {
        if (!ConvertObjIndex(
                obj_vertex.tex_coord,
                tex_coords.size(),
                key.tex_coord
            )) {
            return false;
        }

        key.has_tex_coord =
            true;
    }

    if (obj_vertex.has_normal) {
        if (!ConvertObjIndex(
                obj_vertex.normal,
                normals.size(),
                key.normal
            )) {
            return false;
        }

        key.has_normal =
            true;
    }

    const auto iterator =
        vertex_cache.find(
            key
        );

    if (iterator != vertex_cache.end()) {
        render_index =
            iterator->second;

        return true;
    }

    Vertex vertex{};

    vertex.position =
        positions[
            key.position
        ];

    if (key.has_tex_coord) {
        vertex.tex_coord =
            tex_coords[
                key.tex_coord
            ];
    }

    if (key.has_normal) {
        vertex.normal =
            normals[
                key.normal
            ];
    }

    render_index =
        static_cast<std::uint32_t>(
            render_vertices.size()
        );

    render_vertices.push_back(
        vertex
    );

    vertex_cache.emplace(
        key,
        render_index
    );

    return true;
}

/**
 * @brief Разбирает polygon face и преобразует его в triangles.
 *
 * OBJ позволяет полигоны с произвольным количеством вершин.
 *
 * В Renderer используются triangles, поэтому применяется
 * fan triangulation:
 *
 * 0 1 2
 * 0 2 3
 * 0 3 4
 * ...
 *
 * Одновременно создаются:
 *
 * - edges;
 * - tri_indices;
 * - render_vertices;
 * - render_indices.
 *
 * @param line Строка f.
 * @param positions Все position OBJ.
 * @param tex_coords Все texture coordinates.
 * @param normals Все normals.
 * @param mesh Mesh текущего material part.
 * @param vertex_cache Кэш render-вершин этого part.
 *
 * @return true при успешном разборе.
 */
static bool ParseFaceLine(std::string_view line, const std::vector<Vec3>& positions, const std::vector<Vec2>& tex_coords, const std::vector<Vec3>& normals, ImportedMeshData& mesh, RenderVertexCache& vertex_cache) {
    if (
        line.size() < 2 ||
        line[0] != 'f' ||
        !IsSpace(line[1])
    ) {
        return false;
    }

    const char* current =
        line.data() + 1;

    const char* end =
        line.data() +
        line.size();

    std::vector<ObjVertexIndices> face_indices;

    while (current < end) {
        SkipSpaces(
            current,
            end
        );

        if (
            current >= end ||
            *current == '#'
        ) {
            break;
        }

        ObjVertexIndices indices{};

        if (!ParseFaceVertexIndices(
                current,
                end,
                indices
            )) {
            return false;
        }

        face_indices.push_back(
            indices
        );
    }

    if (face_indices.size() < 3) {
        return false;
    }

    /*
     * Создаём рёбра исходного полигона.
     */
    for (std::size_t index = 0; index < face_indices.size(); ++index) {
        const std::size_t next =
            (index + 1) %
            face_indices.size();

        std::uint32_t first = 0;
        std::uint32_t second = 0;

        if (!ConvertObjIndex(
                face_indices[index].position,
                positions.size(),
                first
            )) {
            return false;
        }

        if (!ConvertObjIndex(
                face_indices[next].position,
                positions.size(),
                second
            )) {
            return false;
        }

        if (first == second) {
            continue;
        }

        if (first > second) {
            std::swap(
                first,
                second
            );
        }

        mesh.edges.emplace_back(
            first,
            second
        );
    }

    /*
     * Triangulation polygon через triangle fan.
     */
    std::uint32_t first_position = 0;

    if (!ConvertObjIndex(
            face_indices.front().position,
            positions.size(),
            first_position
        )) {
        return false;
    }

    for (std::size_t index = 1; index + 1 < face_indices.size(); ++index) {
        std::uint32_t second_position = 0;
        std::uint32_t third_position = 0;

        if (!ConvertObjIndex(
                face_indices[index].position,
                positions.size(),
                second_position
            )) {
            return false;
        }

        if (!ConvertObjIndex(
                face_indices[index + 1].position,
                positions.size(),
                third_position
            )) {
            return false;
        }

        mesh.tri_indices.push_back(
            first_position
        );

        mesh.tri_indices.push_back(
            second_position
        );

        mesh.tri_indices.push_back(
            third_position
        );

        const ObjVertexIndices triangle[3] = {
            face_indices.front(),
            face_indices[index],
            face_indices[index + 1]
        };

        for (const ObjVertexIndices& obj_vertex : triangle) {
            std::uint32_t render_index = 0;

            if (!GetOrCreateRenderVertex(
                    obj_vertex,
                    positions,
                    tex_coords,
                    normals,
                    mesh.render_vertices,
                    vertex_cache,
                    render_index
                )) {
                return false;
            }

            mesh.render_indices.push_back(
                render_index
            );
        }
    }

    return true;
}

/**
 * @brief Завершает формирование ImportedMeshData.
 *
 * Метод:
 *
 * - сохраняет исходные position / normal / UV;
 * - выставляет flags;
 * - удаляет повторяющиеся edges.
 *
 * @param mesh Завершаемый Mesh.
 * @param positions Position всего OBJ.
 * @param tex_coords Texture coordinates всего OBJ.
 * @param normals Normals всего OBJ.
 */
static void FinalizeMeshData(ImportedMeshData& mesh, const std::vector<Vec3>& positions, const std::vector<Vec2>& tex_coords, const std::vector<Vec3>& normals) {
    mesh.positions =
        positions;

    mesh.tex_coords =
        tex_coords;

    mesh.normals =
        normals;

    mesh.has_tex_coords =
        !tex_coords.empty();

    mesh.has_normals =
        !normals.empty();

    std::sort(
        mesh.edges.begin(),
        mesh.edges.end()
    );

    mesh.edges.erase(
        std::unique(
            mesh.edges.begin(),
            mesh.edges.end()
        ),
        mesh.edges.end()
    );
}

/**
 * @brief Загружает простой OBJ в один ImportedMeshData.
 *
 * Эта перегрузка используется там, где материалы
 * и multipart-структура не нужны.
 *
 * Поддерживается:
 *
 * - v;
 * - vt;
 * - vn;
 * - f;
 * - отрицательные OBJ-индексы;
 * - triangulation polygon.
 *
 * @param filename Путь к OBJ.
 * @param mesh_data Результирующий Mesh.
 *
 * @return true, если создана render-геометрия.
 */
bool ObjParser::Parse(const std::string& filename, ImportedMeshData& mesh_data) {
    std::ifstream file(
        filename
    );

    if (!file.is_open()) {
        return false;
    }

    mesh_data =
        ImportedMeshData{};

    std::vector<Vec3> positions;
    std::vector<Vec2> tex_coords;
    std::vector<Vec3> normals;

    RenderVertexCache vertex_cache;

    std::string line;

    while (std::getline(file, line)) {
        const char* current =
            line.data();

        const char* end =
            line.data() +
            line.size();

        SkipSpaces(
            current,
            end
        );

        if (
            current >= end ||
            *current == '#'
        ) {
            continue;
        }

        const std::string_view trimmed_line(
            current,
            static_cast<std::size_t>(
                end - current
            )
        );

        if (trimmed_line.starts_with("vt")) {
            Vec2 tex_coord{};

            if (ParseTexCoordLine(
                    trimmed_line,
                    tex_coord
                )) {
                tex_coords.push_back(
                    tex_coord
                );
            }

            continue;
        }

        if (trimmed_line.starts_with("vn")) {
            Vec3 normal{};

            if (ParseNormalLine(
                    trimmed_line,
                    normal
                )) {
                normals.push_back(
                    normal
                );
            }

            continue;
        }

        if (trimmed_line.starts_with("v")) {
            Vec3 position{};

            if (ParsePositionLine(
                    trimmed_line,
                    position
                )) {
                positions.push_back(
                    position
                );
            }

            continue;
        }

        if (
            trimmed_line.size() >= 2 &&
            trimmed_line[0] == 'f' &&
            IsSpace(trimmed_line[1])
        ) {
            if (!ParseFaceLine(
                    trimmed_line,
                    positions,
                    tex_coords,
                    normals,
                    mesh_data,
                    vertex_cache
                )) {
                return false;
            }

            continue;
        }
    }

    FinalizeMeshData(
        mesh_data,
        positions,
        tex_coords,
        normals
    );

    return
        !mesh_data.render_vertices.empty() &&
        !mesh_data.render_indices.empty();
}

/**
 * @brief Загружает OBJ вместе с multipart-структурой и MTL.
 *
 * Именно эта перегрузка используется ModelImporter.
 *
 * Поддерживается цепочка:
 *
 * OBJ
 * -> mtllib
 * -> MTL
 * -> usemtl
 * -> ImportedMeshPart
 *
 * Каждый уникальный usemtl создаёт отдельный
 * ImportedMeshPart.
 *
 * При повторном появлении того же usemtl
 * геометрия добавляется в уже существующий part.
 *
 * Это важно для SceneObject:
 *
 * SceneObject
 * ├── Material A
 * ├── Material B
 * └── Material C
 *
 * вместо создания нескольких SceneObject.
 *
 * @param filename Путь к OBJ.
 * @param model_data Результирующая модель.
 *
 * @return true, если была загружена хотя бы одна
 * пригодная для рендера часть.
 */
bool ObjParser::Parse(const std::string& filename, ImportedModelData& model_data) {
    std::ifstream file(
        filename
    );

    if (!file.is_open()) {
        return false;
    }

    /*
     * Очищаем предыдущий результат,
     * если ImportedModelData используется повторно.
     */
    model_data.meshes.clear();
    model_data.materials.clear();

    /*
     * OBJ хранит source arrays глобально.
     *
     * Faces во всех material parts ссылаются
     * именно на эти массивы.
     */
    std::vector<Vec3> positions;
    std::vector<Vec2> tex_coords;
    std::vector<Vec3> normals;

    /*
     * Базовая директория используется
     * для разрешения относительного mtllib.
     *
     * Например:
     *
     * /models/vase/vase.obj
     *
     * mtllib vase.mtl
     *
     * превращается в:
     *
     * /models/vase/vase.mtl
     */
    const std::filesystem::path obj_path(
        filename
    );

    const std::filesystem::path obj_directory =
        obj_path.parent_path();

    /*
     * Для каждого material part нужен отдельный
     * RenderVertexCache, потому что render_indices
     * локальны относительно конкретного Mesh.
     */
    std::vector<RenderVertexCache> vertex_caches;

    /*
     * Позволяет повторный usemtl не создавать
     * второй RenderPart с тем же именем.
     *
     * Это также удобно для SceneSerializer,
     * который сопоставляет multipart materials
     * по имени.
     */
    std::unordered_map<std::string, std::size_t>
        part_indices;

    std::string current_material_name;

    /**
     * Возвращает индекс part для текущего материала.
     *
     * Если material ещё не встречался,
     * создаётся новый ImportedMeshPart.
     */
    const auto get_or_create_part = [&]() -> std::size_t {
        const auto existing =
            part_indices.find(
                current_material_name
            );

        if (existing != part_indices.end()) {
            return existing->second;
        }

        ImportedMeshPart part{};

        part.material_name =
            current_material_name;

        const std::size_t index =
            model_data.meshes.size();

        model_data.meshes.push_back(
            std::move(part)
        );

        vertex_caches.emplace_back();

        part_indices.emplace(
            current_material_name,
            index
        );

        return index;
    };

    std::string line;

    while (std::getline(file, line)) {
        const char* current =
            line.data();

        const char* end =
            line.data() +
            line.size();

        SkipSpaces(
            current,
            end
        );

        if (
            current >= end ||
            *current == '#'
        ) {
            continue;
        }

        const std::string_view trimmed_line(
            current,
            static_cast<std::size_t>(
                end - current
            )
        );

        /*
         * Material library.
         *
         * Важная часть исправления:
         *
         * mtllib рассматривается относительно
         * директории OBJ, а не working directory
         * приложения.
         */
        if (
            trimmed_line.size() > 6 &&
            trimmed_line.starts_with("mtllib") &&
            IsSpace(trimmed_line[6])
        ) {
            std::string material_file =
                TrimObjText(
                    std::string(
                        trimmed_line.substr(6)
                    )
                );

            material_file =
                RemoveObjQuotes(
                    material_file
                );

            if (!material_file.empty()) {
                std::filesystem::path material_path(
                    material_file
                );

                if (material_path.is_relative()) {
                    material_path =
                        obj_directory /
                        material_path;
                }

                material_path =
                    material_path.
                        lexically_normal();

                /*
                 * Парсим каждый mtllib отдельно,
                 * после чего добавляем материалы
                 * в общий список модели.
                 *
                 * Поэтому поддерживаются OBJ
                 * с несколькими библиотеками MTL.
                 */
                std::vector<ImportedMaterialData>
                    imported_materials;

                if (MtlParser::Parse(
                        material_path,
                        imported_materials
                    )) {
                    for (ImportedMaterialData& material : imported_materials) {
                        const auto existing =
                            std::find_if(
                                model_data.materials.begin(),
                                model_data.materials.end(),
                                [&material](const ImportedMaterialData& current_material) {
                                    return current_material.name == material.name;
                                }
                            );

                        /*
                         * Повторное имя material не добавляем,
                         * чтобы FindMaterial получал однозначный
                         * результат.
                         */
                        if (existing == model_data.materials.end()) {
                            model_data.materials.push_back(
                                std::move(material)
                            );
                        }
                    }
                }
            }

            continue;
        }

        /*
         * Texture coordinate.
         */
        if (trimmed_line.starts_with("vt")) {
            Vec2 tex_coord{};

            if (ParseTexCoordLine(
                    trimmed_line,
                    tex_coord
                )) {
                tex_coords.push_back(
                    tex_coord
                );
            }

            continue;
        }

        /*
         * Vertex normal.
         */
        if (trimmed_line.starts_with("vn")) {
            Vec3 normal{};

            if (ParseNormalLine(
                    trimmed_line,
                    normal
                )) {
                normals.push_back(
                    normal
                );
            }

            continue;
        }

        /*
         * Vertex position.
         */
        if (trimmed_line.starts_with("v")) {
            Vec3 position{};

            if (ParsePositionLine(
                    trimmed_line,
                    position
                )) {
                positions.push_back(
                    position
                );
            }

            continue;
        }

        /*
         * Выбор материала для следующих faces.
         *
         * Например:
         *
         * usemtl phong1SG
         */
        if (
            trimmed_line.size() > 6 &&
            trimmed_line.starts_with("usemtl") &&
            IsSpace(trimmed_line[6])
        ) {
            current_material_name =
                TrimObjText(
                    std::string(
                        trimmed_line.substr(6)
                    )
                );

            current_material_name =
                RemoveObjQuotes(
                    current_material_name
                );

            continue;
        }

        /*
         * Face добавляется в Mesh того material,
         * который был последним указан через usemtl.
         *
         * Если usemtl отсутствовал, используется
         * part с пустым material_name.
         */
        if (
            trimmed_line.size() >= 2 &&
            trimmed_line[0] == 'f' &&
            IsSpace(trimmed_line[1])
        ) {
            const std::size_t part_index =
                get_or_create_part();

            ImportedMeshPart& part =
                model_data.meshes[
                    part_index
                ];

            RenderVertexCache& vertex_cache =
                vertex_caches[
                    part_index
                ];

            if (!ParseFaceLine(
                    trimmed_line,
                    positions,
                    tex_coords,
                    normals,
                    part.mesh,
                    vertex_cache
                )) {
                return false;
            }

            continue;
        }
    }

    /*
     * Завершаем каждый material part.
     */
    for (ImportedMeshPart& part : model_data.meshes) {
        FinalizeMeshData(
            part.mesh,
            positions,
            tex_coords,
            normals
        );
    }

    /*
     * Удаляем пустые parts.
     *
     * Такое возможно, если usemtl встретился,
     * но после него не было ни одного face.
     */
    model_data.meshes.erase(
        std::remove_if(
            model_data.meshes.begin(),
            model_data.meshes.end(),
            [](const ImportedMeshPart& part) {
                return
                    part.mesh.render_vertices.empty() ||
                    part.mesh.render_indices.empty();
            }
        ),
        model_data.meshes.end()
    );

    return !model_data.meshes.empty();
}