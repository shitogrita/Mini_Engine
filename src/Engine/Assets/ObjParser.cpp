#include "ObjParser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

bool IsSpace(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

void SkipSpaces(const char*& ptr, const char* end) {
    while (ptr < end && IsSpace(*ptr)) {
        ++ptr;
    }
}

bool ParseFloat(const char*& ptr, const char* end, float& value) {
    SkipSpaces(ptr, end);

    const auto [next, error] = std::from_chars(ptr, end, value);

    if (error != std::errc()) {
        return false;
    }

    ptr = next;
    return true;
}

bool ParseInt(const char*& ptr, const char* end, int& value) {
    const auto [next, error] = std::from_chars(ptr, end, value);

    if (error != std::errc()) {
        return false;
    }

    ptr = next;
    return true;
}

struct FaceVertex {
    int position = -1;
    int tex_coord = -1;
    int normal = -1;
};

struct FaceVertexKey {
    int position = -1;
    int tex_coord = -1;
    int normal = -1;

    bool operator==(const FaceVertexKey& other) const {
        return position == other.position &&
               tex_coord == other.tex_coord &&
               normal == other.normal;
    }
};

struct FaceVertexKeyHasher {
    std::size_t operator()(const FaceVertexKey& key) const {
        std::size_t h1 = std::hash<int>{}(key.position);
        std::size_t h2 = std::hash<int>{}(key.tex_coord);
        std::size_t h3 = std::hash<int>{}(key.normal);

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

int ConvertObjIndex(int index, std::size_t size) {
    if (index < 0) {
        return static_cast<int>(size) + index;
    }

    return index - 1;
}

bool IsValidIndex(int index, std::size_t size) {
    return index >= 0 && index < static_cast<int>(size);
}

bool ParseFaceVertexToken(const char* begin,
                          const char* end,
                          FaceVertex& out) {
    const char* ptr = begin;

    int position = 0;
    if (!ParseInt(ptr, end, position)) {
        return false;
    }

    out.position = position;

    if (ptr >= end || *ptr != '/') {
        return true;
    }

    ++ptr;

    if (ptr < end && *ptr != '/') {
        int tex_coord = 0;

        if (!ParseInt(ptr, end, tex_coord)) {
            return false;
        }

        out.tex_coord = tex_coord;
    }

    if (ptr >= end || *ptr != '/') {
        return true;
    }

    ++ptr;

    if (ptr < end) {
        int normal = 0;

        if (!ParseInt(ptr, end, normal)) {
            return false;
        }

        out.normal = normal;
    }

    return true;
}

bool ParseVertexLine(const std::string& line, MeshData& mesh_data) {
    const char* ptr = line.data() + 1;
    const char* end = line.data() + line.size();

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (!ParseFloat(ptr, end, x)) {
        return false;
    }

    if (!ParseFloat(ptr, end, y)) {
        return false;
    }

    if (!ParseFloat(ptr, end, z)) {
        return false;
    }

    mesh_data.positions.push_back(Vec3{x, y, z});

    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    const char* color_ptr = ptr;

    if (ParseFloat(color_ptr, end, r) &&
        ParseFloat(color_ptr, end, g) &&
        ParseFloat(color_ptr, end, b)) {
        mesh_data.colors.push_back(Color3{r, g, b});
        mesh_data.has_colors = true;
    } else {
        mesh_data.colors.push_back(Color3{1.0f, 1.0f, 1.0f});
    }

    return true;
}

bool ParseTexCoordLine(const std::string& line, MeshData& mesh_data) {
    const char* ptr = line.data() + 2;
    const char* end = line.data() + line.size();

    float u = 0.0f;
    float v = 0.0f;

    if (!ParseFloat(ptr, end, u)) {
        return false;
    }

    if (!ParseFloat(ptr, end, v)) {
        return false;
    }

    mesh_data.tex_coords.push_back(Vec2{u, v});
    mesh_data.has_tex_coords = true;

    return true;
}

bool ParseNormalLine(const std::string& line, MeshData& mesh_data) {
    const char* ptr = line.data() + 2;
    const char* end = line.data() + line.size();

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (!ParseFloat(ptr, end, x)) {
        return false;
    }

    if (!ParseFloat(ptr, end, y)) {
        return false;
    }

    if (!ParseFloat(ptr, end, z)) {
        return false;
    }

    mesh_data.normals.push_back(Vec3{x, y, z});
    mesh_data.has_normals = true;

    return true;
}

std::uint32_t GetOrCreateRenderVertex(
    const FaceVertex& face_vertex,
    MeshData& mesh_data,
    std::unordered_map<FaceVertexKey, std::uint32_t, FaceVertexKeyHasher>& vertex_map
) {
    FaceVertexKey key{
        face_vertex.position,
        face_vertex.tex_coord,
        face_vertex.normal
    };

    auto found = vertex_map.find(key);

    if (found != vertex_map.end()) {
        return found->second;
    }

    Vertex vertex{};

    vertex.position = mesh_data.positions[face_vertex.position];

    if (face_vertex.normal >= 0) {
        vertex.normal = mesh_data.normals[face_vertex.normal];
    }

    if (face_vertex.tex_coord >= 0) {
        vertex.tex_coord = mesh_data.tex_coords[face_vertex.tex_coord];
    }

    if (face_vertex.position >= 0 &&
        face_vertex.position < static_cast<int>(mesh_data.colors.size())) {
        vertex.color = mesh_data.colors[face_vertex.position];
    }

    const auto new_index = static_cast<std::uint32_t>(mesh_data.render_vertices.size());

    mesh_data.render_vertices.push_back(vertex);
    vertex_map.emplace(key, new_index);

    return new_index;
}

bool ParseFaceLine(
    const std::string& line,
    MeshData& mesh_data,
    std::unordered_map<FaceVertexKey, std::uint32_t, FaceVertexKeyHasher>& vertex_map
) {
    const char* ptr = line.data() + 1;
    const char* end = line.data() + line.size();

    std::vector<FaceVertex> face_vertices;
    face_vertices.reserve(8);

    while (ptr < end) {
        SkipSpaces(ptr, end);

        if (ptr >= end || *ptr == '#') {
            break;
        }

        const char* token_begin = ptr;

        while (ptr < end && !IsSpace(*ptr)) {
            ++ptr;
        }

        const char* token_end = ptr;

        FaceVertex face_vertex{};

        if (!ParseFaceVertexToken(token_begin, token_end, face_vertex)) {
            return false;
        }

        face_vertex.position =
            ConvertObjIndex(face_vertex.position, mesh_data.positions.size());

        if (!IsValidIndex(face_vertex.position, mesh_data.positions.size())) {
            return false;
        }

        if (face_vertex.tex_coord != -1) {
            face_vertex.tex_coord =
                ConvertObjIndex(face_vertex.tex_coord, mesh_data.tex_coords.size());

            if (!IsValidIndex(face_vertex.tex_coord, mesh_data.tex_coords.size())) {
                return false;
            }
        }

        if (face_vertex.normal != -1) {
            face_vertex.normal =
                ConvertObjIndex(face_vertex.normal, mesh_data.normals.size());

            if (!IsValidIndex(face_vertex.normal, mesh_data.normals.size())) {
                return false;
            }
        }

        face_vertices.push_back(face_vertex);
    }

    if (face_vertices.size() < 3) {
        return false;
    }

    for (std::size_t i = 0; i < face_vertices.size(); ++i) {
        std::size_t j = (i + 1) % face_vertices.size();

        auto v1 = static_cast<std::uint32_t>(face_vertices[i].position);
        auto v2 = static_cast<std::uint32_t>(face_vertices[j].position);

        if (v1 == v2) {
            continue;
        }

        if (v1 > v2) {
            std::swap(v1, v2);
        }

        mesh_data.edges.emplace_back(v1, v2);
    }

    const auto position_v0 = static_cast<std::uint32_t>(face_vertices[0].position);

    for (std::size_t i = 1; i + 1 < face_vertices.size(); ++i) {
        mesh_data.tri_indices.push_back(position_v0);
        mesh_data.tri_indices.push_back(static_cast<std::uint32_t>(face_vertices[i].position));
        mesh_data.tri_indices.push_back(static_cast<std::uint32_t>(face_vertices[i + 1].position));

        const std::uint32_t rv0 =
            GetOrCreateRenderVertex(face_vertices[0], mesh_data, vertex_map);
        const std::uint32_t rv1 =
            GetOrCreateRenderVertex(face_vertices[i], mesh_data, vertex_map);
        const std::uint32_t rv2 =
            GetOrCreateRenderVertex(face_vertices[i + 1], mesh_data, vertex_map);

        mesh_data.render_indices.push_back(rv0);
        mesh_data.render_indices.push_back(rv1);
        mesh_data.render_indices.push_back(rv2);
    }

    return true;
}

} // namespace

bool ObjParser::Parse(const std::string& filename, MeshData& mesh_data) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    mesh_data = MeshData{};

    file.seekg(0, std::ios::end);
    const std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size > 0) {
        const auto size = static_cast<std::size_t>(file_size);

        mesh_data.positions.reserve(size / 32);
        mesh_data.normals.reserve(size / 48);
        mesh_data.tex_coords.reserve(size / 48);
        mesh_data.colors.reserve(size / 32);
        mesh_data.edges.reserve(size / 24);
        mesh_data.tri_indices.reserve(size / 16);
        mesh_data.render_vertices.reserve(size / 32);
        mesh_data.render_indices.reserve(size / 16);
    }

    std::unordered_map<FaceVertexKey, std::uint32_t, FaceVertexKeyHasher> vertex_map;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line[0] == 'v' && line.size() > 1 && IsSpace(line[1])) {
            ParseVertexLine(line, mesh_data);
            continue;
        }

        if (line[0] == 'v' &&
            line.size() > 2 &&
            line[1] == 't' &&
            IsSpace(line[2])) {
            ParseTexCoordLine(line, mesh_data);
            continue;
        }

        if (line[0] == 'v' &&
            line.size() > 2 &&
            line[1] == 'n' &&
            IsSpace(line[2])) {
            ParseNormalLine(line, mesh_data);
            continue;
        }

        if (line[0] == 'f' && line.size() > 1 && IsSpace(line[1])) {
            ParseFaceLine(line, mesh_data, vertex_map);
            continue;
        }
    }

    std::sort(mesh_data.edges.begin(), mesh_data.edges.end());
    mesh_data.edges.erase(
        std::unique(mesh_data.edges.begin(), mesh_data.edges.end()),
        mesh_data.edges.end()
    );

    return !mesh_data.positions.empty() &&
           !mesh_data.render_vertices.empty() &&
           !mesh_data.render_indices.empty();
}