#include "Engine/Renderer/PrimitiveGenerator.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace {

constexpr float kPi = 3.14159265358979323846f;

Vertex MakeVertex(const Vec3& position, const Vec3& normal, const Vec2& uv) {
    Vertex vertex{};
    vertex.position = position;
    vertex.normal = normal;
    vertex.tex_coord = uv;
    return vertex;
}

void AddQuad(ImportedMeshData& data, const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec3& normal) {
    const std::uint32_t base = static_cast<std::uint32_t>(data.render_vertices.size());

    data.render_vertices.push_back(MakeVertex(v0, normal, {0.0f, 0.0f}));
    data.render_vertices.push_back(MakeVertex(v1, normal, {1.0f, 0.0f}));
    data.render_vertices.push_back(MakeVertex(v2, normal, {1.0f, 1.0f}));
    data.render_vertices.push_back(MakeVertex(v3, normal, {0.0f, 1.0f}));

    data.render_indices.push_back(base + 0);
    data.render_indices.push_back(base + 1);
    data.render_indices.push_back(base + 2);

    data.render_indices.push_back(base + 0);
    data.render_indices.push_back(base + 2);
    data.render_indices.push_back(base + 3);
}

}


ImportedMeshData PrimitiveGenerator::CreateCube(float size) {
    if (size <= 0.0f) {
        throw std::invalid_argument("Cube size must be greater than zero");
    }

    ImportedMeshData data;
    const float h = size * 0.5f;

    data.positions = {
        {-h, -h, -h},
        { h, -h, -h},
        { h,  h, -h},
        {-h,  h, -h},
        {-h, -h,  h},
        { h, -h,  h},
        { h,  h,  h},
        {-h,  h,  h}
    };

    // Front +Z
    AddQuad(data,
        {-h, -h,  h},
        { h, -h,  h},
        { h,  h,  h},
        {-h,  h,  h},
        {0.0f, 0.0f, 1.0f}
    );

    // Back -Z
    AddQuad(data,
        { h, -h, -h},
        {-h, -h, -h},
        {-h,  h, -h},
        { h,  h, -h},
        {0.0f, 0.0f, -1.0f}
    );

    // Right +X
    AddQuad(data,
        { h, -h,  h},
        { h, -h, -h},
        { h,  h, -h},
        { h,  h,  h},
        {1.0f, 0.0f, 0.0f}
    );

    // Left -X
    AddQuad(data,
        {-h, -h, -h},
        {-h, -h,  h},
        {-h,  h,  h},
        {-h,  h, -h},
        {-1.0f, 0.0f, 0.0f}
    );

    // Top +Y
    AddQuad(data,
        {-h, h,  h},
        { h, h,  h},
        { h, h, -h},
        {-h, h, -h},
        {0.0f, 1.0f, 0.0f}
    );

    // Bottom -Y
    AddQuad(data,
        {-h, -h, -h},
        { h, -h, -h},
        { h, -h,  h},
        {-h, -h,  h},
        {0.0f, -1.0f, 0.0f}
    );

    return data;
}


ImportedMeshData PrimitiveGenerator::CreatePlane(float size) {
    if (size <= 0.0f) {
        throw std::invalid_argument("Plane size must be greater than zero");
    }

    ImportedMeshData data;
    const float h = size * 0.5f;

    const Vec3 v0{-h, 0.0f,  h};
    const Vec3 v1{ h, 0.0f,  h};
    const Vec3 v2{ h, 0.0f, -h};
    const Vec3 v3{-h, 0.0f, -h};

    data.positions = {v0, v1, v2, v3};

    AddQuad(data, v0, v1, v2, v3, {0.0f, 1.0f, 0.0f});

    return data;
}


ImportedMeshData PrimitiveGenerator::CreateSphere(float radius, unsigned int segments, unsigned int rings) {
    if (radius <= 0.0f) {
        throw std::invalid_argument("Sphere radius must be greater than zero");
    }

    if (segments < 3) {
        throw std::invalid_argument("Sphere must have at least 3 segments");
    }

    if (rings < 2) {
        throw std::invalid_argument("Sphere must have at least 2 rings");
    }

    ImportedMeshData data;

    const unsigned int vertices_per_ring = segments + 1;

    for (unsigned int ring = 0; ring <= rings; ++ring) {
        const float v = static_cast<float>(ring) / static_cast<float>(rings);
        const float phi = v * kPi;

        const float sin_phi = std::sin(phi);
        const float cos_phi = std::cos(phi);

        for (unsigned int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float theta = u * 2.0f * kPi;

            const float sin_theta = std::sin(theta);
            const float cos_theta = std::cos(theta);

            const Vec3 normal{
                sin_phi * cos_theta,
                cos_phi,
                sin_phi * sin_theta
            };

            const Vec3 position{
                normal.x * radius,
                normal.y * radius,
                normal.z * radius
            };

            data.positions.push_back(position);
            data.render_vertices.push_back(
                MakeVertex(position, normal, {u, v})
            );
        }
    }

    for (unsigned int ring = 0; ring < rings; ++ring) {
        for (unsigned int segment = 0; segment < segments; ++segment) {
            const std::uint32_t current = ring * vertices_per_ring + segment;
            const std::uint32_t next = current + vertices_per_ring;

            if (ring != 0) {
                data.render_indices.push_back(current);
                data.render_indices.push_back(current + 1);
                data.render_indices.push_back(next);
            }

            if (ring != rings - 1) {
                data.render_indices.push_back(current + 1);
                data.render_indices.push_back(next + 1);
                data.render_indices.push_back(next);
            }
        }
    }

    return data;
}