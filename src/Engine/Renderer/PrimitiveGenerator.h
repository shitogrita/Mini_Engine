#pragma once

#include "Engine/Assets/ImportedMeshData.h"

/**
 * @brief Генератор встроенных геометрических примитивов.
 *
 * PrimitiveGenerator создаёт геометрию примитивов
 * непосредственно в памяти без загрузки модели из файла.
 *
 * Результат возвращается в формате ImportedMeshData,
 * который также используется системой импорта моделей.
 *
 * Это позволяет передавать сгенерированную геометрию
 * в тот же дальнейший pipeline,
 * что и данные, полученные через ObjParser.
 *
 * Класс не хранит собственного состояния
 * и используется только через статические методы.
 */
class PrimitiveGenerator {
public:
    /**
     * @brief Создаёт куб с центром в начале координат.
     *
     * Куб имеет одинаковый размер по осям X, Y и Z.
     *
     * Каждая грань получает собственные render vertices,
     * что позволяет хранить отдельные нормали
     * и UV-координаты для каждой стороны.
     *
     * @param size Длина стороны куба.
     *
     * @return ImportedMeshData с геометрией куба.
     *
     * @throws std::invalid_argument Если size меньше или равен 0.
     */
    static ImportedMeshData CreateCube(float size = 1.0f);

    /**
     * @brief Создаёт квадратную плоскость в плоскости XZ.
     *
     * Центр плоскости находится в начале координат,
     * а её нормаль направлена вдоль положительной оси Y.
     *
     * @param size Длина стороны плоскости.
     *
     * @return ImportedMeshData с геометрией плоскости.
     *
     * @throws std::invalid_argument Если size меньше или равен 0.
     */
    static ImportedMeshData CreatePlane(float size = 1.0f);

    /**
     * @brief Создаёт UV-сферу с заданным радиусом
     * и уровнем детализации.
     *
     * Параметр segments определяет количество делений
     * вокруг вертикальной оси.
     *
     * Параметр rings определяет количество делений
     * между верхним и нижним полюсами.
     *
     * @param radius Радиус сферы.
     * @param segments Количество сегментов вокруг сферы.
     * @param rings Количество вертикальных делений.
     *
     * @return ImportedMeshData с геометрией сферы.
     *
     * @throws std::invalid_argument Если radius меньше или равен 0.
     * @throws std::invalid_argument Если segments меньше 3.
     * @throws std::invalid_argument Если rings меньше 2.
     */
    static ImportedMeshData CreateSphere(float radius = 0.5f, unsigned int segments = 32, unsigned int rings = 16);
};