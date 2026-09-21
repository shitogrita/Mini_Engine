#include "Engine/Assets/ObjParser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace {

struct ObjVertexIndices {
    int position = 0;
    int tex_coord = 0;
    int normal = 0;

    bool has_tex_coord = false;
    bool has_normal = false;
};

struct RenderVertexKey {
    std::uint32_t position = 0;
    std::uint32_t tex_coord = 0;
    std::uint32_t normal = 0;

    bool has_tex_coord = false;
    bool has_normal = false;

    bool operator==(const RenderVertexKey& other) const {
        return position == other.position &&
               tex_coord == other.tex_coord &&
               normal == other.normal &&
               has_tex_coord == other.has_tex_coord &&
               has_normal == other.has_normal;
    }
};

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

using RenderVertexCache = std::unordered_map<RenderVertexKey, std::uint32_t, RenderVertexKeyHash>;

bool IsSpace(char symbol) {
    return std::isspace(static_cast<unsigned char>(symbol)) != 0;
}

void SkipSpaces(const char*& current, const char* end) {
    while (current < end && IsSpace(*current)) {
        ++current;
    }
}

bool ParseFloat(const char*& current, const char* end, float& value) {
    SkipSpaces(current, end);

    if (current >= end) {
        return false;
    }

    const auto [next, error] = std::from_chars(current, end, value);

    if (error != std::errc{}) {
        return false;
    }

    current = next;
    return true;
}

bool ParseInteger(const char*& current, const char* end, int& value) {
    SkipSpaces(current, end);

    if (current >= end) {
        return false;
    }

    const auto [next, error] = std::from_chars(current, end, value);

    if (error != std::errc{}) {
        return false;
    }

    current = next;
    return true;
}

bool ParsePositionLine(std::string_view line, Vec3& position) {
    if (line.size() < 2 || line[0] != 'v' || !IsSpace(line[1])) {
        return false;
    }

    const char* current = line.data() + 1;
    const char* end = line.data() + line.size();

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

    position = Vec3{x, y, z};
    return true;
}

bool ParseTexCoordLine(std::string_view line, Vec2& tex_coord) {
    if (line.size() < 3 || line[0] != 'v' || line[1] != 't' || !IsSpace(line[2])) {
        return false;
    }

    const char* current = line.data() + 2;
    const char* end = line.data() + line.size();

    float u = 0.0f;
    float v = 0.0f;

    if (!ParseFloat(current, end, u)) {
        return false;
    }

    if (!ParseFloat(current, end, v)) {
        return false;
    }

    tex_coord = Vec2{u, v};
    return true;
}

bool ParseNormalLine(std::string_view line, Vec3& normal) {
    if (line.size() < 3 || line[0] != 'v' || line[1] != 'n' || !IsSpace(line[2])) {
        return false;
    }

    const char* current = line.data() + 2;
    const char* end = line.data() + line.size();

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

    normal = Vec3{x, y, z};
    return true;
}

/**
 * @brief Преобразует индекс OBJ в индекс массива C++.
 *
 * OBJ использует индексацию с 1.
 * Отрицательные индексы считаются от конца уже прочитанного массива.
 */
bool ConvertObjIndex(int obj_index, std::size_t elements_count, std::uint32_t& result) {
    int index = 0;

    if (obj_index > 0) {
        index = obj_index - 1;
    } else if (obj_index < 0) {
        index = static_cast<int>(elements_count) + obj_index;
    } else {
        return false;
    }

    if (index < 0 || index >= static_cast<int>(elements_count)) {
        return false;
    }

    result = static_cast<std::uint32_t>(index);
    return true;
}

/**
 * @brief Разбирает одну запись вершины внутри строки f.
 *
 * Поддерживаются:
 * v
 * v/vt
 * v//vn
 * v/vt/vn
 */
bool ParseFaceVertexIndices(const char*& current, const char* end, ObjVertexIndices& indices) {
    SkipSpaces(current, end);

    if (!ParseInteger(current, end, indices.position)) {
        return false;
    }

    // Только position: v
    if (current >= end || IsSpace(*current)) {
        return true;
    }

    if (*current != '/') {
        return false;
    }

    ++current;

    // Если сразу встретили второй '/', используется формат v//vn.
    if (current < end && *current != '/') {
        if (!ParseInteger(current, end, indices.tex_coord)) {
            return false;
        }

        indices.has_tex_coord = true;
    }

    // Формат v/vt.
    if (current >= end || IsSpace(*current)) {
        return true;
    }

    if (*current != '/') {
        return false;
    }

    ++current;

    // Формат v//vn или v/vt/vn.
    if (!ParseInteger(current, end, indices.normal)) {
        return false;
    }

    indices.has_normal = true;
    return true;
}

/**
 * @brief Возвращает существующий индекс GPU-вершины
 * или создаёт новую вершину.
 *
 * GPU-вершина определяется комбинацией:
 * position + texture coordinate + normal.
 */
bool GetOrCreateRenderVertex(
    const ObjVertexIndices& obj_vertex,
    const std::vector<Vec3>& positions,
    const std::vector<Vec2>& tex_coords,
    const std::vector<Vec3>& normals,
    std::vector<Vertex>& render_vertices,
    RenderVertexCache& vertex_cache,
    std::uint32_t& render_index) {

    RenderVertexKey key{};

    if (!ConvertObjIndex(obj_vertex.position, positions.size(), key.position)) {
        return false;
    }

    if (obj_vertex.has_tex_coord) {
        if (!ConvertObjIndex(obj_vertex.tex_coord, tex_coords.size(), key.tex_coord)) {
            return false;
        }

        key.has_tex_coord = true;
    }

    if (obj_vertex.has_normal) {
        if (!ConvertObjIndex(obj_vertex.normal, normals.size(), key.normal)) {
            return false;
        }

        key.has_normal = true;
    }

    const auto iterator = vertex_cache.find(key);

    if (iterator != vertex_cache.end()) {
        render_index = iterator->second;
        return true;
    }

    Vertex vertex{};
    vertex.position = positions[key.position];

    if (key.has_tex_coord) {
        vertex.tex_coord = tex_coords[key.tex_coord];
    }

    if (key.has_normal) {
        vertex.normal = normals[key.normal];
    }

    render_index = static_cast<std::uint32_t>(render_vertices.size());

    render_vertices.push_back(vertex);
    vertex_cache.emplace(key, render_index);

    return true;
}

/**
 * @brief Разбирает одну грань OBJ.
 *
 * Формирует:
 * - геометрические рёбра;
 * - геометрические индексы треугольников;
 * - GPU-вершины;
 * - GPU-индексы.
 */
bool ParseFaceLine(
    std::string_view line,
    const std::vector<Vec3>& positions,
    const std::vector<Vec2>& tex_coords,
    const std::vector<Vec3>& normals,
    std::vector<Edge>& edges,
    std::vector<std::uint32_t>& tri_indices,
    std::vector<Vertex>& render_vertices,
    std::vector<std::uint32_t>& render_indices,
    RenderVertexCache& vertex_cache) {

    if (line.size() < 2 || line[0] != 'f' || !IsSpace(line[1])) {
        return false;
    }

    const char* current = line.data() + 1;
    const char* end = line.data() + line.size();

    std::vector<ObjVertexIndices> face_indices;

    while (current < end) {
        SkipSpaces(current, end);

        if (current >= end || *current == '#') {
            break;
        }

        ObjVertexIndices indices{};

        if (!ParseFaceVertexIndices(current, end, indices)) {
            return false;
        }

        face_indices.push_back(indices);
    }

    if (face_indices.size() < 3) {
        return false;
    }

    /*
     * Формируем геометрические рёбра.
     *
     * Для рёбер используются только positions,
     * потому что UV и normal не изменяют положение
     * геометрического ребра в пространстве.
     */
    for (std::size_t i = 0; i < face_indices.size(); ++i) {
        const std::size_t next = (i + 1) % face_indices.size();

        std::uint32_t first = 0;
        std::uint32_t second = 0;

        if (!ConvertObjIndex(face_indices[i].position, positions.size(), first)) {
            return false;
        }

        if (!ConvertObjIndex(face_indices[next].position, positions.size(), second)) {
            return false;
        }

        if (first == second) {
            continue;
        }

        if (first > second) {
            std::swap(first, second);
        }

        edges.emplace_back(first, second);
    }

    /*
     * Триангуляция грани веером.
     *
     * f A B C D
     *
     * превращается в:
     *
     * A B C
     * A C D
     */
    for (std::size_t i = 1; i + 1 < face_indices.size(); ++i) {
        const ObjVertexIndices triangle[3] = {
            face_indices.front(),
            face_indices[i],
            face_indices[i + 1]
        };

        /*
         * tri_indices описывает исходную геометрию
         * через индексы positions.
         */
        for (const ObjVertexIndices& obj_vertex : triangle) {
            std::uint32_t position_index = 0;

            if (!ConvertObjIndex(obj_vertex.position, positions.size(), position_index)) {
                return false;
            }

            tri_indices.push_back(position_index);
        }

        /*
         * render_indices индексирует полноценные GPU-вершины.
         *
         * Здесь учитывается вся комбинация:
         * position + UV + normal.
         */
        for (const ObjVertexIndices& obj_vertex : triangle) {
            std::uint32_t render_index = 0;

            if (!GetOrCreateRenderVertex(
                    obj_vertex,
                    positions,
                    tex_coords,
                    normals,
                    render_vertices,
                    vertex_cache,
                    render_index)) {
                return false;
            }

            render_indices.push_back(render_index);
        }
    }

    return true;
}

} // namespace

bool ObjParser::Parse(const std::string& filename, ImportedMeshData& mesh_data) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        return false;
    }

    mesh_data.positions.clear();
    mesh_data.normals.clear();
    mesh_data.tex_coords.clear();
    mesh_data.colors.clear();

    mesh_data.edges.clear();
    mesh_data.tri_indices.clear();

    mesh_data.render_vertices.clear();
    mesh_data.render_indices.clear();

    mesh_data.has_normals = false;
    mesh_data.has_tex_coords = false;
    mesh_data.has_colors = false;

    /*
     * Кеш существует на протяжении импорта всего OBJ.
     *
     * Благодаря этому одинаковые комбинации v/vt/vn,
     * встречающиеся в разных faces, используют одну GPU-вершину.
     */
    RenderVertexCache vertex_cache;

    std::string line;

    while (std::getline(file, line)) {
        const char* current = line.data();
        const char* end = line.data() + line.size();

        SkipSpaces(current, end);

        if (current >= end || *current == '#') {
            continue;
        }

        const std::string_view trimmed_line(current, static_cast<std::size_t>(end - current));

        if (trimmed_line.starts_with("vt")) {
            Vec2 tex_coord{};

            if (ParseTexCoordLine(trimmed_line, tex_coord)) {
                mesh_data.tex_coords.push_back(tex_coord);
                mesh_data.has_tex_coords = true;
            }

            continue;
        }

        if (trimmed_line.starts_with("vn")) {
            Vec3 normal{};

            if (ParseNormalLine(trimmed_line, normal)) {
                mesh_data.normals.push_back(normal);
                mesh_data.has_normals = true;
            }

            continue;
        }

        if (trimmed_line.starts_with("v")) {
            Vec3 position{};

            if (ParsePositionLine(trimmed_line, position)) {
                mesh_data.positions.push_back(position);
            }

            continue;
        }

        if (trimmed_line.starts_with("f")) {
            if (!ParseFaceLine(
                    trimmed_line,
                    mesh_data.positions,
                    mesh_data.tex_coords,
                    mesh_data.normals,
                    mesh_data.edges,
                    mesh_data.tri_indices,
                    mesh_data.render_vertices,
                    mesh_data.render_indices,
                    vertex_cache)) {
                return false;
            }
        }
    }

    /*
     * Одно геометрическое ребро может встретиться у нескольких faces.
     *
     * После приведения каждого Edge к виду (min, max)
     * сортируем массив и удаляем повторения.
     */
    std::sort(mesh_data.edges.begin(), mesh_data.edges.end());

    mesh_data.edges.erase(
        std::unique(mesh_data.edges.begin(), mesh_data.edges.end()),
        mesh_data.edges.end()
    );

    return !mesh_data.render_vertices.empty() && !mesh_data.render_indices.empty();
}