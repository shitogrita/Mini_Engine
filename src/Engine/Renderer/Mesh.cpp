#include "Engine/Renderer/Mesh.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

Mesh::Mesh(const ImportedMeshData& mesh_data) {
    index_count_ = static_cast<unsigned int>(
        mesh_data.render_indices.size()
    );

    // Создаём OpenGL-объекты.
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glGenBuffers(1, &instance_vbo_);

    // VAO будет запоминать конфигурацию нашего Mesh.
    glBindVertexArray(vao_);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            mesh_data.render_vertices.size() * sizeof(Vertex)
        ),
        mesh_data.render_vertices.data(),
        GL_STATIC_DRAW
    );

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            mesh_data.render_indices.size() * sizeof(std::uint32_t)
        ),
        mesh_data.render_indices.data(),
        GL_STATIC_DRAW
    );

    // layout(location = 0) in vec3 aPos;
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(
            offsetof(Vertex, position)
        )
    );

    glEnableVertexAttribArray(0);

    // layout(location = 1) in vec3 aNormal;
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(
            offsetof(Vertex, normal)
        )
    );

    glEnableVertexAttribArray(1);

    // layout(location = 2) in vec2 aTexCoord;
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(
            offsetof(Vertex, tex_coord)
        )
    );

    glEnableVertexAttribArray(2);

    // layout(location = 3) in vec3 aColor;
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(
            offsetof(Vertex, color)
        )
    );

    glEnableVertexAttribArray(3);

    /**
     * ВАЖНО:
     *
     * Attributes 4-7 здесь НЕ включаем.
     *
     * instance_vbo_ пока не содержит Model Matrix.
     * Они будут настроены только в SetInstanceMatrices().
     */
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Mesh::~Mesh() {
    Destroy();
}

Mesh::Mesh(Mesh&& other) noexcept {
    vao_ = std::exchange(other.vao_, 0);
    vbo_ = std::exchange(other.vbo_, 0);
    ebo_ = std::exchange(other.ebo_, 0);
    instance_vbo_ = std::exchange(other.instance_vbo_, 0);
    index_count_ = std::exchange(other.index_count_, 0);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    // Сначала освобождаем ресурсы,
    // которыми владел текущий Mesh.
    Destroy();

    // Забираем ресурсы other.
    vao_ = std::exchange(other.vao_, 0);
    vbo_ = std::exchange(other.vbo_, 0);
    ebo_ = std::exchange(other.ebo_, 0);
    instance_vbo_ = std::exchange(other.instance_vbo_, 0);
    index_count_ = std::exchange(other.index_count_, 0);

    return *this;
}

void Mesh::Bind() const {
    glBindVertexArray(vao_);
}

unsigned int Mesh::GetIndexCount() const {
    return index_count_;
}

void Mesh::SetInstanceMatrices(const std::vector<Matrix4>& matrices) const {
    if (instance_vbo_ == 0 || matrices.empty()) {
        return;
    }

    std::vector<float> packed_matrices;

    packed_matrices.resize(
        matrices.size() * 16
    );

    /**
     * Преобразуем Matrix4 в четыре column vectors,
     * которые ожидает GLSL mat4 attribute.
     */
    for (std::size_t matrix_index = 0; matrix_index < matrices.size(); ++matrix_index) {
        const Matrix4& matrix = matrices[matrix_index];

        float* destination =
            packed_matrices.data() +
            matrix_index * 16;

        for (std::size_t column = 0; column < 4; ++column) {
            for (std::size_t row = 0; row < 4; ++row) {
                destination[column * 4 + row] =
                    matrix[row][column];
            }
        }
    }

    /**
     * Сначала загружаем реальные данные.
     */
    glBindBuffer(
        GL_ARRAY_BUFFER,
        instance_vbo_
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            packed_matrices.size() *
            sizeof(float)
        ),
        packed_matrices.data(),
        GL_DYNAMIC_DRAW
    );

    /**
     * Только теперь связываем instance buffer с VAO
     * и включаем locations 4-7.
     */
    glBindVertexArray(vao_);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        instance_vbo_
    );

    constexpr GLsizei matrix_stride =
        static_cast<GLsizei>(
            16 * sizeof(float)
        );

    for (unsigned int column = 0; column < 4; ++column) {
        const unsigned int location =
            4 + column;

        glVertexAttribPointer(
            location,
            4,
            GL_FLOAT,
            GL_FALSE,
            matrix_stride,
            reinterpret_cast<void*>(
                static_cast<std::uintptr_t>(
                    column * 4 * sizeof(float)
                )
            )
        );

        glEnableVertexAttribArray(
            location
        );

        glVertexAttribDivisor(
            location,
            1
        );
    }

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(0);
}

void Mesh::DisableInstanceAttributes() const {
    glBindVertexArray(vao_);

    for (unsigned int location = 4; location <= 7; ++location) {
        glDisableVertexAttribArray(
            location
        );
    }

    glBindVertexArray(0);
}

void Mesh::Destroy() {
    if (instance_vbo_ != 0) {
        glDeleteBuffers(
            1,
            &instance_vbo_
        );

        instance_vbo_ = 0;
    }

    if (ebo_ != 0) {
        glDeleteBuffers(
            1,
            &ebo_
        );

        ebo_ = 0;
    }

    if (vbo_ != 0) {
        glDeleteBuffers(
            1,
            &vbo_
        );

        vbo_ = 0;
    }

    if (vao_ != 0) {
        glDeleteVertexArrays(
            1,
            &vao_
        );

        vao_ = 0;
    }

    index_count_ = 0;
}