#include "Engine/Assets/ObjParser.h"

#include "Engine/Assets/ImportedModelData.h"
#include "Engine/Assets/MtlParser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <filesystem>
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

using RenderVertexCache =
    std::unordered_map<RenderVertexKey, std::uint32_t, RenderVertexKeyHash>;

bool IsSpace(char symbol) {
    return std::isspace(static_cast<unsigned char>(symbol)) != 0;
}

void SkipSpaces(const char*& current, const char* end) {
    while (current < end && IsSpace(*current)) {
        ++current;
    }
}

std::string_view TrimLeft(std::string_view line) {
    std::size_t position = 0;

    while (position < line.size() && IsSpace(line[position])) {
        ++position;
    }

    return line.substr(position);
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

    if (!ParseFloat(current, end, position.x)) {
        return false;
    }

    if (!ParseFloat(current, end, position.y)) {
        return false;
    }

    if (!ParseFloat(current, end, position.z)) {
        return false;
    }

    return true;
}

bool ParseTexCoordLine(std::string_view line, Vec2& tex_coord) {
    if (line.size() < 3 || line[0] != 'v' || line[1] != 't' || !IsSpace(line[2])) {
        return false;
    }

    const char* current = line.data() + 2;
    const char* end = line.data() + line.size();

    if (!ParseFloat(current, end, tex_coord.x)) {
        return false;
    }

    if (!ParseFloat(current, end, tex_coord.y)) {
        return false;
    }

    return true;
}

bool ParseNormalLine(std::string_view line, Vec3& normal) {
    if (line.size() < 3 || line[0] != 'v' || line[1] != 'n' || !IsSpace(line[2])) {
        return false;
    }

    const char* current = line.data() + 2;
    const char* end = line.data() + line.size();

    if (!ParseFloat(current, end, normal.x)) {
        return false;
    }

    if (!ParseFloat(current, end, normal.y)) {
        return false;
    }

    if (!ParseFloat(current, end, normal.z)) {
        return false;
    }

    return true;
}

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

bool ParseFaceVertexIndices(const char*& current, const char* end, ObjVertexIndices& indices) {
    SkipSpaces(current, end);

    if (!ParseInteger(current, end, indices.position)) {
        return false;
    }

    if (current >= end || IsSpace(*current)) {
        return true;
    }

    if (*current != '/') {
        return false;
    }

    ++current;

    if (current < end && *current != '/') {
        if (!ParseInteger(current, end, indices.tex_coord)) {
            return false;
        }

        indices.has_tex_coord = true;
    }

    if (current >= end || IsSpace(*current)) {
        return true;
    }

    if (*current != '/') {
        return false;
    }

    ++current;

    if (!ParseInteger(current, end, indices.normal)) {
        return false;
    }

    indices.has_normal = true;
    return true;
}

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

bool ParseFaceLine(
    std::string_view line,
    const std::vector<Vec3>& positions,
    const std::vector<Vec2>& tex_coords,
    const std::vector<Vec3>& normals,
    ImportedMeshData& mesh_data,
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

        mesh_data.edges.emplace_back(first, second);
    }

    for (std::size_t i = 1; i + 1 < face_indices.size(); ++i) {
        const ObjVertexIndices triangle[3] = {
            face_indices.front(),
            face_indices[i],
            face_indices[i + 1]
        };

        for (const ObjVertexIndices& obj_vertex : triangle) {
            std::uint32_t position_index = 0;

            if (!ConvertObjIndex(obj_vertex.position, positions.size(), position_index)) {
                return false;
            }

            mesh_data.tri_indices.push_back(position_index);
        }

        for (const ObjVertexIndices& obj_vertex : triangle) {
            std::uint32_t render_index = 0;

            if (!GetOrCreateRenderVertex(
                    obj_vertex,
                    positions,
                    tex_coords,
                    normals,
                    mesh_data.render_vertices,
                    vertex_cache,
                    render_index)) {
                return false;
            }

            mesh_data.render_indices.push_back(render_index);
        }
    }

    return true;
}

void FinishMesh(ImportedMeshData& mesh_data) {
    std::sort(mesh_data.edges.begin(), mesh_data.edges.end());

    mesh_data.edges.erase(
        std::unique(mesh_data.edges.begin(), mesh_data.edges.end()),
        mesh_data.edges.end()
    );
}

ImportedMeshPart& GetOrCreateMeshPart(
    ImportedModelData& model_data,
    const std::string& material_name,
    const std::vector<Vec3>& positions,
    const std::vector<Vec2>& tex_coords,
    const std::vector<Vec3>& normals) {

    for (ImportedMeshPart& part : model_data.meshes) {
        if (part.material_name == material_name) {
            return part;
        }
    }

    model_data.meshes.emplace_back();

    ImportedMeshPart& part = model_data.meshes.back();

    part.material_name = material_name;
    part.mesh.positions = positions;
    part.mesh.tex_coords = tex_coords;
    part.mesh.normals = normals;

    part.mesh.has_tex_coords = !tex_coords.empty();
    part.mesh.has_normals = !normals.empty();

    return part;
}

} // namespace

bool ObjParser::Parse(const std::string& filename, ImportedMeshData& mesh_data) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        return false;
    }

    mesh_data = ImportedMeshData{};

    RenderVertexCache vertex_cache;

    std::string line;

    while (std::getline(file, line)) {
        const std::string_view trimmed_line = TrimLeft(line);

        if (trimmed_line.empty() || trimmed_line[0] == '#') {
            continue;
        }

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
                    mesh_data,
                    vertex_cache)) {
                return false;
            }
        }
    }

    FinishMesh(mesh_data);

    return !mesh_data.render_vertices.empty() &&
           !mesh_data.render_indices.empty();
}

bool ObjParser::Parse(const std::string& filename, ImportedModelData& model_data) {
    const std::filesystem::path obj_path(filename);

    std::ifstream file(obj_path);

    if (!file.is_open()) {
        return false;
    }

    model_data = ImportedModelData{};

    /*
     * Эти массивы принадлежат всему OBJ.
     *
     * Индексы внутри f относятся именно к ним,
     * а не к отдельному материалу.
     */
    std::vector<Vec3> positions;
    std::vector<Vec2> tex_coords;
    std::vector<Vec3> normals;

    /*
     * usemtl изменяет это имя.
     *
     * Пустая строка означает, что faces пока
     * не привязаны к какому-либо материалу.
     */
    std::string current_material_name;

    /*
     * У каждого MeshPart свой кеш.
     *
     * Одинаковая комбинация v/vt/vn внутри одного
     * материала переиспользует GPU-вершину.
     */
    std::unordered_map<std::string, RenderVertexCache> vertex_caches;

    std::string line;

    while (std::getline(file, line)) {
        const std::string_view trimmed_line = TrimLeft(line);

        if (trimmed_line.empty() || trimmed_line[0] == '#') {
            continue;
        }

        /*
         * mtllib сообщает имя MTL-файла.
         *
         * Путь считается относительно OBJ.
         */
        if (trimmed_line.starts_with("mtllib ")) {
            const std::string mtl_name(TrimLeft(trimmed_line.substr(6)));

            if (!mtl_name.empty()) {
                const std::filesystem::path mtl_path =
                    (obj_path.parent_path() / mtl_name).lexically_normal();

                std::vector<ImportedMaterialData> materials =
                    MtlParser::Parse(mtl_path);

                model_data.materials.insert(
                    model_data.materials.end(),
                    materials.begin(),
                    materials.end()
                );
            }

            continue;
        }

        /*
         * usemtl выбирает материал для следующих faces.
         */
        if (trimmed_line.starts_with("usemtl ")) {
            current_material_name =
                std::string(TrimLeft(trimmed_line.substr(6)));

            continue;
        }

        if (trimmed_line.starts_with("vt")) {
            Vec2 tex_coord{};

            if (ParseTexCoordLine(trimmed_line, tex_coord)) {
                tex_coords.push_back(tex_coord);
            }

            continue;
        }

        if (trimmed_line.starts_with("vn")) {
            Vec3 normal{};

            if (ParseNormalLine(trimmed_line, normal)) {
                normals.push_back(normal);
            }

            continue;
        }

        if (trimmed_line.starts_with("v")) {
            Vec3 position{};

            if (ParsePositionLine(trimmed_line, position)) {
                positions.push_back(position);
            }

            continue;
        }

        if (trimmed_line.starts_with("f")) {
            ImportedMeshPart& part = GetOrCreateMeshPart(
                model_data,
                current_material_name,
                positions,
                tex_coords,
                normals
            );

            RenderVertexCache& vertex_cache =
                vertex_caches[current_material_name];

            if (!ParseFaceLine(
                    trimmed_line,
                    positions,
                    tex_coords,
                    normals,
                    part.mesh,
                    vertex_cache)) {
                return false;
            }
        }
    }

    for (ImportedMeshPart& part : model_data.meshes) {
        /*
         * К моменту окончания файла могли появиться v/vt/vn
         * после создания MeshPart.
         *
         * Поэтому сохраняем полные исходные массивы.
         */
        part.mesh.positions = positions;
        part.mesh.tex_coords = tex_coords;
        part.mesh.normals = normals;

        part.mesh.has_tex_coords = !tex_coords.empty();
        part.mesh.has_normals = !normals.empty();

        FinishMesh(part.mesh);
    }

    return !model_data.meshes.empty();
}