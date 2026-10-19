#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Math/matrix_types.h"

#include <vector>

/**
 * @brief Представляет геометрию, загруженную в OpenGL GPU buffers.
 *
 * Mesh является владельцем OpenGL-ресурсов,
 * необходимых для отрисовки геометрии:
 *
 * - VAO — конфигурация vertex attributes;
 * - VBO — данные вершин;
 * - EBO — индексные данные;
 * - instance VBO — model matrices для instanced rendering.
 *
 * Геометрия создаётся из ImportedMeshData,
 * содержащего подготовленные render vertices
 * и render indices.
 *
 * Класс реализует RAII для OpenGL-ресурсов:
 * ресурсы создаются в конструкторе
 * и освобождаются в деструкторе.
 *
 * Копирование Mesh запрещено,
 * поскольку несколько объектов не должны владеть
 * одними и теми же OpenGL handles.
 *
 * При этом Mesh поддерживает move semantics,
 * позволяя безопасно передавать владение GPU-ресурсами.
 */
class Mesh {
public:
    /**
     * @brief Создаёт Mesh на основе импортированных данных геометрии.
     *
     * Конструктор создаёт VAO, VBO, EBO и instance VBO,
     * после чего загружает vertex/index data в GPU memory
     * и настраивает основные vertex attributes.
     *
     * Для выполнения конструктора необходим
     * активный OpenGL-контекст.
     *
     * @param mesh_data Данные геометрии,
     * используемые для создания GPU-представления Mesh.
     */
    explicit Mesh(const ImportedMeshData& mesh_data);

    /**
     * @brief Освобождает принадлежащие Mesh OpenGL-ресурсы.
     *
     * Удаление ресурсов выполняется через Destroy().
     */
    ~Mesh();

    /**
     * @brief Запрещает копирующее создание Mesh.
     *
     * OpenGL handles представляют ресурсы,
     * которыми должен владеть только один объект Mesh.
     */
    Mesh(const Mesh&) = delete;

    /**
     * @brief Запрещает копирующее присваивание Mesh.
     */
    Mesh& operator=(const Mesh&) = delete;

    /**
     * @brief Передаёт владение GPU-ресурсами от другого Mesh.
     *
     * После перемещения исходный объект
     * больше не владеет переданными OpenGL handles.
     *
     * @param other Mesh, ресурсы которого необходимо получить.
     */
    Mesh(Mesh&& other) noexcept;

    /**
     * @brief Перемещающий оператор присваивания.
     *
     * Сначала освобождает текущие ресурсы объекта,
     * после чего принимает OpenGL handles от other.
     *
     * @param other Mesh, ресурсы которого необходимо получить.
     *
     * @return Ссылка на текущий Mesh.
     */
    Mesh& operator=(Mesh&& other) noexcept;

    /**
     * @brief Делает VAO этого Mesh активным.
     *
     * После вызова OpenGL использует конфигурацию
     * vertex attributes и EBO, сохранённые в vao_.
     */
    void Bind() const;

    /**
     * @brief Возвращает количество индексов Mesh.
     *
     * Значение соответствует количеству элементов
     * в render_indices, использованных при создании Mesh.
     *
     * @return Количество индексов геометрии.
     */
    unsigned int GetIndexCount() const;

    /**
     * @brief Загружает model matrices для instanced rendering.
     *
     * Матрицы преобразуются в формат,
     * необходимый для передачи GLSL mat4
     * через четыре последовательных vertex attributes.
     *
     * Используются locations 4-7:
     *
     * - location 4 — первый столбец Matrix4;
     * - location 5 — второй столбец Matrix4;
     * - location 6 — третий столбец Matrix4;
     * - location 7 — четвёртый столбец Matrix4.
     *
     * Attributes 4-7 включаются только после того,
     * как instance_vbo_ получает реальные данные.
     *
     * Для каждого attribute устанавливается divisor = 1,
     * поэтому одна model matrix используется для одного instance.
     *
     * @param matrices Model matrices экземпляров Mesh.
     */
    void SetInstanceMatrices(const std::vector<Matrix4>& matrices) const;

    /**
     * @brief Отключает instance vertex attributes.
     *
     * Метод отключает vertex attribute arrays
     * с locations 4-7 в VAO данного Mesh.
     *
     * Основные attributes геометрии
     * с locations 0-3 остаются включёнными.
     */
    void DisableInstanceAttributes() const;

private:
    /**
     * @brief OpenGL Vertex Array Object.
     *
     * Хранит конфигурацию vertex attributes Mesh.
     */
    unsigned int vao_ = 0;

    /**
     * @brief OpenGL Vertex Buffer Object.
     *
     * Содержит данные структуры Vertex.
     */
    unsigned int vbo_ = 0;

    /**
     * @brief OpenGL Element Buffer Object.
     *
     * Содержит индексы геометрии.
     */
    unsigned int ebo_ = 0;

    /**
     * @brief OpenGL buffer для instance model matrices.
     *
     * Используется при instanced rendering.
     */
    unsigned int instance_vbo_ = 0;

    /**
     * @brief Количество индексов Mesh.
     *
     * Используется Renderer при indexed draw calls.
     */
    unsigned int index_count_ = 0;

    /**
     * @brief Освобождает принадлежащие Mesh OpenGL-ресурсы.
     *
     * Метод удаляет instance VBO, EBO, VBO и VAO,
     * после чего сбрасывает соответствующие handles
     * и index_count_ в 0.
     *
     * Используется деструктором
     * и move assignment operator.
     */
    void Destroy();
};