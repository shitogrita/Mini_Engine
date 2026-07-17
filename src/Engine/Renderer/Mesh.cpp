#include "Engine/Renderer/Mesh.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#include <cstddef>
#include <utility>

Mesh::Mesh(const ImportedMeshData& mesh_data)
{
    index_count_ = static_cast<unsigned int>(mesh_data.tri_indices.size());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<long>(mesh_data.render_vertices.size() * sizeof(Vertex)),
        mesh_data.render_vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<long>(mesh_data.tri_indices.size() * sizeof(std::uint32_t)),
        mesh_data.tri_indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tex_coord)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

Mesh::~Mesh()
{
    Destroy();
}

Mesh::Mesh(Mesh&& other) noexcept
{
    vao_ = std::exchange(other.vao_, 0);
    vbo_ = std::exchange(other.vbo_, 0);
    ebo_ = std::exchange(other.ebo_, 0);
    index_count_ = std::exchange(other.index_count_, 0);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        Destroy();

        vao_ = std::exchange(other.vao_, 0);
        vbo_ = std::exchange(other.vbo_, 0);
        ebo_ = std::exchange(other.ebo_, 0);
        index_count_ = std::exchange(other.index_count_, 0);
    }

    return *this;
}

void Mesh::Draw() const
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<int>(index_count_), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::Destroy()
{
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }

    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    index_count_ = 0;
}