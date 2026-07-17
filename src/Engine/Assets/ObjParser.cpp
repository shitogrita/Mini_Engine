#include "Engine/Assets/ObjParser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

bool IsSpace(char symbol)
{
    return std::isspace(static_cast<unsigned char>(symbol)) != 0;
}

void SkipSpaces(const char*& current, const char* end)
{
    while (current < end && IsSpace(*current)) {
        ++current;
    }
}

bool ParseFloat(const char*& current, const char* end, float& value)
{
    SkipSpaces(current, end);

    if (current >= end) {
        return false;
    }

    const auto [next, error] =
        std::from_chars(current, end, value);

    if (error != std::errc{}) {
        return false;
    }

    current = next;
    return true;
}

bool ParseInteger(const char*& current, const char* end, int& value)
{
    SkipSpaces(current, end);

    if (current >= end) {
        return false;
    }

    const auto [next, error] =
        std::from_chars(current, end, value);

    if (error != std::errc{}) {
        return false;
    }

    current = next;
    return true;
}

bool ParsePositionLine(std::string_view line, Vec3& position)
{
    if (line.size() < 2 ||
        line[0] != 'v' ||
        !IsSpace(line[1])) {
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

bool ParseTexCoordLine(std::string_view line, Vec2& tex_coord)
{
    if (line.size() < 3 ||
        line[0] != 'v' ||
        line[1] != 't' ||
        !IsSpace(line[2])) {
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

bool ParseNormalLine(std::string_view line, Vec3& normal)
{
    if (line.size() < 3 ||
        line[0] != 'v' ||
        line[1] != 'n' ||
        !IsSpace(line[2])) {
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

bool ConvertObjIndex(
    int obj_index,
    std::size_t positions_count,
    std::uint32_t& result)
{
    int index = 0;

    if (obj_index > 0) {
        index = obj_index - 1;
    } else if (obj_index < 0) {
        index =
            static_cast<int>(positions_count) + obj_index;
    } else {
        return false;
    }

    if (index < 0 ||
        index >= static_cast<int>(positions_count)) {
        return false;
    }

    result = static_cast<std::uint32_t>(index);
    return true;
}

bool ParseFaceVertexPositionIndex(
    const char*& current,
    const char* end,
    std::size_t positions_count,
    std::uint32_t& result)
{
    int obj_index = 0;

    if (!ParseInteger(current, end, obj_index)) {
        return false;
    }

    if (!ConvertObjIndex(
            obj_index,
            positions_count,
            result)) {
        return false;
    }

    /*
     * После индекса позиции может идти:
     *
     * 1
     * 1/2
     * 1//3
     * 1/2/3
     *
     * На данном этапе индексы UV и нормалей
     * пропускаются. Мы сохраняем только индекс позиции.
     */
    while (current < end && !IsSpace(*current)) {
        ++current;
    }

    return true;
}

bool ParseFaceLine(
    std::string_view line,
    std::size_t positions_count,
    std::vector<Edge>& edges,
    std::vector<std::uint32_t>& tri_indices)
{
    if (line.size() < 2 ||
        line[0] != 'f' ||
        !IsSpace(line[1])) {
        return false;
    }

    const char* current = line.data() + 1;
    const char* end = line.data() + line.size();

    std::vector<std::uint32_t> face_indices;

    while (current < end) {
        SkipSpaces(current, end);

        if (current >= end || *current == '#') {
            break;
        }

        std::uint32_t position_index = 0;

        if (!ParseFaceVertexPositionIndex(
                current,
                end,
                positions_count,
                position_index)) {
            return false;
        }

        face_indices.push_back(position_index);
    }

    if (face_indices.size() < 3) {
        return false;
    }

    for (std::size_t i = 0;
         i < face_indices.size();
         ++i) {
        const std::size_t next =
            (i + 1) % face_indices.size();

        std::uint32_t first = face_indices[i];
        std::uint32_t second = face_indices[next];

        if (first == second) {
            continue;
        }

        if (first > second) {
            std::swap(first, second);
        }

        edges.emplace_back(first, second);
    }

    /*
     * Триангуляция веером.
     *
     * Для грани:
     *
     * f 1 2 3 4
     *
     * создаются треугольники:
     *
     * 1 2 3
     * 1 3 4
     */
    const std::uint32_t first =
        face_indices.front();

    for (std::size_t i = 1;
         i + 1 < face_indices.size();
         ++i) {
        tri_indices.push_back(first);
        tri_indices.push_back(face_indices[i]);
        tri_indices.push_back(face_indices[i + 1]);
        // Метод веера корректен для выпуклых полигонов.
        // Для сложного вогнутого полигона он может создать неправильные треугольники
    }

    return true;
}
} // namespace

bool ObjParser::Parse(
    const std::string &filename,
    ImportedMeshData &mesh_data) {
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

    std::string line;

    while (std::getline(file, line)) {
        const char* current = line.data();
        const char* end = line.data() + line.size();

        SkipSpaces(current, end);

        if (current >= end || *current == '#') {
            continue;
        }

        const std::string_view trimmed_line(
            current,
            static_cast<std::size_t>(end - current)
        );

        if (trimmed_line.starts_with("vt")) {
            Vec2 tex_coord{};

            if (ParseTexCoordLine(
                    trimmed_line,
                    tex_coord)) {
                mesh_data.tex_coords.push_back(tex_coord);
                mesh_data.has_tex_coords = true;
            }

            continue;
        }

        if (trimmed_line.starts_with("vn")) {
            Vec3 normal{};

            if (ParseNormalLine(
                    trimmed_line,
                    normal)) {
                mesh_data.normals.push_back(normal);
                mesh_data.has_normals = true;
            }

            continue;
        }

        if (trimmed_line.starts_with("v")) {
            Vec3 position{};

            if (ParsePositionLine(
                    trimmed_line,
                    position)) {
                mesh_data.positions.push_back(position);
            }

            continue;
        }

        if (trimmed_line.starts_with("f")) {
            ParseFaceLine(
                trimmed_line,
                mesh_data.positions.size(),
                mesh_data.edges,
                mesh_data.tri_indices
            );
        }
    }

    std::sort(
        mesh_data.edges.begin(),
        mesh_data.edges.end()
    );

    mesh_data.edges.erase(
        std::unique(
            mesh_data.edges.begin(),
            mesh_data.edges.end()
        ),
        mesh_data.edges.end()
    );

    return !mesh_data.positions.empty();
}