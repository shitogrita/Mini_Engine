#include "Engine/Renderer/Mesh.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/**
 * @brief Создаёт GPU-представление геометрии на основе ImportedMeshData.
 *
 * Конструктор создаёт OpenGL-ресурсы, необходимые для рендеринга Mesh:
 *
 * - VAO — хранит конфигурацию vertex attributes;
 * - VBO — содержит данные вершин;
 * - EBO — содержит индексы;
 * - instance_vbo_ — предназначен для хранения model matrices
 *   при instanced rendering.
 *
 * Данные из ImportedMeshData загружаются в GPU buffers.
 *
 * Вершинные атрибуты Vertex связываются со следующими locations:
 *
 * - location 0 — position;
 * - location 1 — normal;
 * - location 2 — tex_coord;
 * - location 3 — color.
 *
 * Instance attributes 4-7 в конструкторе не включаются.
 * Они настраиваются только после загрузки instance matrices
 * через SetInstanceMatrices().
 *
 * Для выполнения конструктора должен существовать
 * активный OpenGL-контекст.
 *
 * @param mesh_data Подготовленные данные геометрии,
 * используемые для создания GPU buffers.
 */
Mesh::Mesh(const ImportedMeshData& mesh_data) {
    /**
     * Сохраняем количество индексов,
     * используемое при последующей indexed draw operation.
     */
    index_count_ = static_cast<unsigned int>(
        mesh_data.render_indices.size()
    );

    /**
     * Создаём OpenGL-объекты Mesh.
     */
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glGenBuffers(1, &instance_vbo_);

    /**
     * VAO запоминает конфигурацию vertex attributes
     * и связанный с ним element buffer.
     */
    glBindVertexArray(vao_);

    /**
     * Загружаем vertex data в VBO.
     */
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            mesh_data.render_vertices.size() * sizeof(Vertex)
        ),
        mesh_data.render_vertices.data(),
        GL_STATIC_DRAW
    );

    /**
     * Загружаем index data в EBO.
     *
     * Binding GL_ELEMENT_ARRAY_BUFFER выполняется
     * при активном VAO, поэтому эта связь сохраняется в VAO.
     */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            mesh_data.render_indices.size() * sizeof(std::uint32_t)
        ),
        mesh_data.render_indices.data(),
        GL_STATIC_DRAW
    );

    /**
     * layout(location = 0) in vec3 aPos;
     *
     * Position занимает три float-компоненты
     * внутри структуры Vertex.
     */
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

    /**
     * layout(location = 1) in vec3 aNormal;
     *
     * Normal занимает три float-компоненты.
     */
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

    /**
     * layout(location = 2) in vec2 aTexCoord;
     *
     * Texture coordinates занимают две float-компоненты.
     */
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

    /**
     * layout(location = 3) in vec3 aColor;
     *
     * Vertex color занимает три float-компоненты.
     */
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
     * Attributes 4-7 здесь намеренно не включаются.
     *
     * Они предназначены для передачи Matrix4
     * при instanced rendering.
     *
     * instance_vbo_ на этом этапе ещё не содержит
     * model matrices, поэтому настройка выполняется
     * позже в SetInstanceMatrices().
     */
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * @brief Освобождает OpenGL-ресурсы Mesh.
 *
 * Реальное удаление VAO и buffers делегируется Destroy().
 */
Mesh::~Mesh() {
    Destroy();
}

/**
 * @brief Перемещающий конструктор Mesh.
 *
 * Передаёт владение OpenGL handles
 * от other текущему объекту.
 *
 * std::exchange() одновременно:
 *
 * - возвращает прежний handle;
 * - обнуляет соответствующее поле other.
 *
 * Благодаря этому перемещённый объект больше
 * не считается владельцем переданных OpenGL-ресурсов.
 *
 * @param other Mesh, ресурсы которого передаются текущему объекту.
 */
Mesh::Mesh(Mesh&& other) noexcept {
    vao_ = std::exchange(other.vao_, 0);
    vbo_ = std::exchange(other.vbo_, 0);
    ebo_ = std::exchange(other.ebo_, 0);
    instance_vbo_ = std::exchange(other.instance_vbo_, 0);
    index_count_ = std::exchange(other.index_count_, 0);
}

/**
 * @brief Перемещающий оператор присваивания.
 *
 * Перед передачей новых OpenGL handles
 * текущие ресурсы объекта освобождаются через Destroy().
 *
 * После этого владение ресурсами other
 * переносится в текущий Mesh.
 *
 * @param other Mesh, ресурсы которого необходимо получить.
 *
 * @return Ссылка на текущий Mesh.
 */
Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    /**
     * Сначала освобождаем OpenGL-ресурсы,
     * которыми уже владеет текущий Mesh.
     */
    Destroy();

    /**
     * Передаём владение ресурсами other.
     *
     * Поля other одновременно обнуляются,
     * чтобы его деструктор не удалил
     * переданные текущему объекту ресурсы.
     */
    vao_ = std::exchange(other.vao_, 0);
    vbo_ = std::exchange(other.vbo_, 0);
    ebo_ = std::exchange(other.ebo_, 0);
    instance_vbo_ = std::exchange(other.instance_vbo_, 0);
    index_count_ = std::exchange(other.index_count_, 0);

    return *this;
}

/**
 * @brief Делает VAO данного Mesh активным.
 *
 * После вызова последующие draw operations
 * используют vertex/index configuration,
 * сохранённую в vao_.
 */
void Mesh::Bind() const {
    glBindVertexArray(vao_);
}

/**
 * @brief Возвращает количество индексов Mesh.
 *
 * Значение соответствует количеству элементов,
 * загруженных из render_indices при создании Mesh.
 *
 * @return Количество индексов геометрии.
 */
unsigned int Mesh::GetIndexCount() const {
    return index_count_;
}

/**
 * @brief Загружает model matrices для instanced rendering.
 *
 * Каждая Matrix4 преобразуется из внутреннего представления
 * matrix[row][column] в последовательность четырёх столбцов,
 * которую ожидает GLSL mat4 vertex attribute.
 *
 * После упаковки данные загружаются в instance_vbo_
 * с использованием GL_DYNAMIC_DRAW.
 *
 * Матрица занимает четыре последовательных vertex attributes:
 *
 * - location 4 — column 0;
 * - location 5 — column 1;
 * - location 6 — column 2;
 * - location 7 — column 3.
 *
 * Для каждого из этих attributes устанавливается divisor = 1,
 * поэтому новая matrix используется один раз на instance,
 * а не один раз на vertex.
 *
 * Если instance_vbo_ не создан или matrices пуст,
 * метод завершает работу без изменения OpenGL state.
 *
 * @param matrices Model matrices отдельных instances.
 */
void Mesh::SetInstanceMatrices(const std::vector<Matrix4>& matrices) const {
    if (instance_vbo_ == 0 || matrices.empty()) {
        return;
    }

    /**
     * Временный CPU-buffer для последовательного хранения
     * всех instance matrices.
     *
     * Каждая Matrix4 занимает 16 значений float.
     */
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
     * Загружаем подготовленные instance data
     * в отдельный VBO.
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
     * После загрузки данных связываем instance_vbo_
     * с VAO данного Mesh и настраиваем attributes 4-7.
     */
    glBindVertexArray(vao_);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        instance_vbo_
    );

    /**
     * Полный размер одной Matrix4 в instance buffer.
     */
    constexpr GLsizei matrix_stride =
        static_cast<GLsizei>(
            16 * sizeof(float)
        );

    /**
     * GLSL mat4 передаётся через четыре vec4 attributes.
     *
     * Каждый следующий location начинается
     * с очередного столбца матрицы.
     */
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

        /**
         * Attribute изменяется один раз на instance.
         */
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

/**
 * @brief Отключает instance matrix attributes в VAO.
 *
 * Метод отключает vertex attribute arrays
 * с locations 4-7.
 *
 * Остальные vertex attributes Mesh
 * остаются без изменений.
 */
void Mesh::DisableInstanceAttributes() const {
    glBindVertexArray(vao_);

    for (unsigned int location = 4; location <= 7; ++location) {
        glDisableVertexAttribArray(
            location
        );
    }

    glBindVertexArray(0);
}

/**
 * @brief Освобождает OpenGL-ресурсы Mesh.
 *
 * Метод удаляет созданные объектом:
 *
 * - instance VBO;
 * - EBO;
 * - VBO;
 * - VAO.
 *
 * Перед удалением каждого ресурса проверяется,
 * что соответствующий OpenGL handle не равен 0.
 *
 * После удаления handles и index_count_
 * сбрасываются в нулевые значения.
 *
 * Это также позволяет безопасно вызывать Destroy()
 * из move assignment и деструктора
 * для уже перемещённого Mesh.
 */
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