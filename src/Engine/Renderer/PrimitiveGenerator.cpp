#include "Engine/Renderer/PrimitiveGenerator.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace {

/**
 * @brief Число π, используемое при генерации сферической геометрии.
 */
constexpr float kPi = 3.14159265358979323846f;

/**
 * @brief Создаёт Vertex из позиции, нормали и UV-координат.
 *
 * Вспомогательная функция используется генераторами примитивов
 * для формирования render_vertices внутри ImportedMeshData.
 *
 * Остальные поля Vertex сохраняют значения,
 * полученные при value initialization через Vertex{}.
 *
 * @param position Позиция вершины.
 * @param normal Нормаль вершины.
 * @param uv Текстурные координаты вершины.
 *
 * @return Сформированный Vertex.
 */
Vertex MakeVertex(const Vec3& position, const Vec3& normal, const Vec2& uv) {
    Vertex vertex{};

    vertex.position = position;
    vertex.normal = normal;
    vertex.tex_coord = uv;

    return vertex;
}

/**
 * @brief Добавляет четырёхугольную грань в render geometry.
 *
 * Quad задаётся четырьмя вершинами:
 *
 * v0, v1, v2, v3.
 *
 * Для каждой вершины создаётся отдельный Vertex
 * с одинаковой normal и собственными UV-координатами:
 *
 * - v0 -> (0, 0);
 * - v1 -> (1, 0);
 * - v2 -> (1, 1);
 * - v3 -> (0, 1).
 *
 * Quad разбивается на два треугольника:
 *
 * - v0, v1, v2;
 * - v0, v2, v3.
 *
 * @param data Структура, в которую добавляется геометрия.
 * @param v0 Первая вершина quad.
 * @param v1 Вторая вершина quad.
 * @param v2 Третья вершина quad.
 * @param v3 Четвёртая вершина quad.
 * @param normal Нормаль всей грани.
 */
void AddQuad(ImportedMeshData& data, const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec3& normal) {
    /**
     * Индекс первой вершины нового quad
     * внутри общего массива render_vertices.
     */
    const std::uint32_t base = static_cast<std::uint32_t>(
        data.render_vertices.size()
    );

    data.render_vertices.push_back(
        MakeVertex(v0, normal, {0.0f, 0.0f})
    );

    data.render_vertices.push_back(
        MakeVertex(v1, normal, {1.0f, 0.0f})
    );

    data.render_vertices.push_back(
        MakeVertex(v2, normal, {1.0f, 1.0f})
    );

    data.render_vertices.push_back(
        MakeVertex(v3, normal, {0.0f, 1.0f})
    );

    /**
     * Первый треугольник quad.
     */
    data.render_indices.push_back(base + 0);
    data.render_indices.push_back(base + 1);
    data.render_indices.push_back(base + 2);

    /**
     * Второй треугольник quad.
     */
    data.render_indices.push_back(base + 0);
    data.render_indices.push_back(base + 2);
    data.render_indices.push_back(base + 3);
}

}

/**
 * @brief Генерирует куб заданного размера.
 *
 * Куб располагается относительно начала координат
 * и имеет одинаковую длину по осям X, Y и Z.
 *
 * Параметр size задаёт полную длину стороны,
 * поэтому координаты каждой стороны находятся
 * на расстоянии size / 2 от центра.
 *
 * В positions сохраняются восемь геометрических углов куба.
 *
 * Для render_vertices каждая из шести граней
 * получает собственные четыре вершины.
 * Такое разделение позволяет каждой грани иметь
 * собственную нормаль и UV-координаты.
 *
 * Куб состоит из шести quad:
 *
 * - Front +Z;
 * - Back -Z;
 * - Right +X;
 * - Left -X;
 * - Top +Y;
 * - Bottom -Y.
 *
 * Каждый quad разбивается на два треугольника.
 *
 * @param size Длина стороны куба.
 *
 * @return ImportedMeshData с геометрией куба.
 *
 * @throws std::invalid_argument Если size меньше или равен 0.
 */
ImportedMeshData PrimitiveGenerator::CreateCube(float size) {
    if (size <= 0.0f) {
        throw std::invalid_argument("Cube size must be greater than zero");
    }

    ImportedMeshData data;

    /**
     * Половина размера используется,
     * поскольку куб центрирован относительно начала координат.
     */
    const float h = size * 0.5f;

    /**
     * Восемь уникальных геометрических углов куба.
     */
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

    /**
     * Front +Z.
     */
    AddQuad(
        data,
        {-h, -h,  h},
        { h, -h,  h},
        { h,  h,  h},
        {-h,  h,  h},
        {0.0f, 0.0f, 1.0f}
    );

    /**
     * Back -Z.
     */
    AddQuad(
        data,
        { h, -h, -h},
        {-h, -h, -h},
        {-h,  h, -h},
        { h,  h, -h},
        {0.0f, 0.0f, -1.0f}
    );

    /**
     * Right +X.
     */
    AddQuad(
        data,
        { h, -h,  h},
        { h, -h, -h},
        { h,  h, -h},
        { h,  h,  h},
        {1.0f, 0.0f, 0.0f}
    );

    /**
     * Left -X.
     */
    AddQuad(
        data,
        {-h, -h, -h},
        {-h, -h,  h},
        {-h,  h,  h},
        {-h,  h, -h},
        {-1.0f, 0.0f, 0.0f}
    );

    /**
     * Top +Y.
     */
    AddQuad(
        data,
        {-h, h,  h},
        { h, h,  h},
        { h, h, -h},
        {-h, h, -h},
        {0.0f, 1.0f, 0.0f}
    );

    /**
     * Bottom -Y.
     */
    AddQuad(
        data,
        {-h, -h, -h},
        { h, -h, -h},
        { h, -h,  h},
        {-h, -h,  h},
        {0.0f, -1.0f, 0.0f}
    );

    return data;
}

/**
 * @brief Генерирует квадратную плоскость заданного размера.
 *
 * Плоскость располагается в плоскости XZ
 * на уровне Y = 0.
 *
 * Центр плоскости совпадает с началом координат.
 *
 * Нормаль направлена вдоль положительной оси Y:
 *
 * (0, 1, 0).
 *
 * Геометрия состоит из одного quad,
 * разбитого на два треугольника.
 *
 * @param size Полная длина стороны плоскости.
 *
 * @return ImportedMeshData с геометрией плоскости.
 *
 * @throws std::invalid_argument Если size меньше или равен 0.
 */
ImportedMeshData PrimitiveGenerator::CreatePlane(float size) {
    if (size <= 0.0f) {
        throw std::invalid_argument("Plane size must be greater than zero");
    }

    ImportedMeshData data;

    /**
     * Половина размера плоскости относительно её центра.
     */
    const float h = size * 0.5f;

    const Vec3 v0{-h, 0.0f,  h};
    const Vec3 v1{ h, 0.0f,  h};
    const Vec3 v2{ h, 0.0f, -h};
    const Vec3 v3{-h, 0.0f, -h};

    data.positions = {
        v0,
        v1,
        v2,
        v3
    };

    AddQuad(
        data,
        v0,
        v1,
        v2,
        v3,
        {0.0f, 1.0f, 0.0f}
    );

    return data;
}

/**
 * @brief Генерирует UV-сферу.
 *
 * Сфера строится параметрически
 * с использованием углов phi и theta.
 *
 * Параметр rings определяет количество делений
 * от верхнего полюса до нижнего.
 *
 * Параметр segments определяет количество делений
 * вокруг вертикальной оси.
 *
 * Для каждой вершины вычисляются:
 *
 * - нормаль;
 * - позиция;
 * - UV-координаты.
 *
 * Нормаль единичной сферы одновременно используется
 * для получения позиции:
 *
 * position = normal * radius.
 *
 * Для корректного UV seam создаётся
 * segments + 1 вершин в каждом ring.
 *
 * Последняя вершина кольца геометрически совпадает
 * с первой, но имеет другое значение U.
 *
 * После генерации вершин соседние rings
 * соединяются треугольниками через render_indices.
 *
 * На верхнем и нижнем полюсах один из двух
 * возможных треугольников для каждого segment
 * намеренно не создаётся.
 *
 * @param radius Радиус сферы.
 * @param segments Количество сегментов вокруг сферы.
 * @param rings Количество вертикальных делений.
 *
 * @return ImportedMeshData с геометрией UV-сферы.
 *
 * @throws std::invalid_argument Если radius меньше или равен 0.
 * @throws std::invalid_argument Если segments меньше 3.
 * @throws std::invalid_argument Если rings меньше 2.
 */
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

    /**
     * Дополнительная вершина в каждом ring
     * необходима для замыкания UV seam.
     */
    const unsigned int vertices_per_ring = segments + 1;

    /**
     * Генерируем вершины сферы.
     *
     * ring изменяет вертикальный угол phi
     * от 0 до PI.
     */
    for (unsigned int ring = 0; ring <= rings; ++ring) {
        const float v =
            static_cast<float>(ring) /
            static_cast<float>(rings);

        const float phi = v * kPi;

        const float sin_phi = std::sin(phi);
        const float cos_phi = std::cos(phi);

        /**
         * segment изменяет угол theta
         * вокруг сферы от 0 до 2 * PI.
         */
        for (unsigned int segment = 0; segment <= segments; ++segment) {
            const float u =
                static_cast<float>(segment) /
                static_cast<float>(segments);

            const float theta = u * 2.0f * kPi;

            const float sin_theta = std::sin(theta);
            const float cos_theta = std::cos(theta);

            /**
             * Единичная нормаль точки сферы.
             */
            const Vec3 normal{
                sin_phi * cos_theta,
                cos_phi,
                sin_phi * sin_theta
            };

            /**
             * Позиция получается масштабированием
             * единичной нормали на radius.
             */
            const Vec3 position{
                normal.x * radius,
                normal.y * radius,
                normal.z * radius
            };

            data.positions.push_back(position);

            data.render_vertices.push_back(
                MakeVertex(
                    position,
                    normal,
                    {u, v}
                )
            );
        }
    }

    /**
     * Формируем triangle indices
     * между соседними rings.
     */
    for (unsigned int ring = 0; ring < rings; ++ring) {
        for (unsigned int segment = 0; segment < segments; ++segment) {
            /**
             * current — вершина текущего ring.
             * next — соответствующая вершина следующего ring.
             */
            const std::uint32_t current =
                ring * vertices_per_ring +
                segment;

            const std::uint32_t next =
                current +
                vertices_per_ring;

            /**
             * Верхний полюс не получает первый
             * из двух обычных треугольников.
             */
            if (ring != 0) {
                data.render_indices.push_back(current);
                data.render_indices.push_back(current + 1);
                data.render_indices.push_back(next);
            }

            /**
             * Нижний полюс не получает второй
             * из двух обычных треугольников.
             */
            if (ring != rings - 1) {
                data.render_indices.push_back(current + 1);
                data.render_indices.push_back(next + 1);
                data.render_indices.push_back(next);
            }
        }
    }

    return data;
}