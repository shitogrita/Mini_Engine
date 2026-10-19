#include "BoundingBox.h"

#include <algorithm>

/**
 * @brief Строит BoundingBox по набору точек.
 *
 * Метод вычисляет минимальные и максимальные координаты
 * по каждой из трёх осей:
 *
 * - X;
 * - Y;
 * - Z.
 *
 * Полученные значения формируют
 * осево-ориентированный BoundingBox:
 *
 * min — минимальные координаты;
 * max — максимальные координаты.
 *
 * Если переданный набор точек пуст,
 * возвращается BoundingBox{},
 * то есть коробка с нулевыми значениями
 * min и max.
 *
 * @param points Набор точек,
 * по которым необходимо определить границы.
 *
 * @return BoundingBox, содержащий все переданные точки.
 */
BoundingBox BoundingBox::FromPoints(const std::vector<Vec3>& points) {
    /**
     * Если геометрия не содержит точек,
     * реальные пространственные границы определить невозможно.
     *
     * В этом случае возвращаем BoundingBox
     * со значениями по умолчанию:
     *
     * min = {0, 0, 0}
     * max = {0, 0, 0}
     */
    if (points.empty()) {
        return BoundingBox{};
    }

    /**
     * Первую точку используем
     * как начальное значение обеих границ.
     *
     * Далее minimum и maximum
     * последовательно расширяются
     * при обработке остальных точек.
     */
    Vec3 minimum =
        points.front();

    Vec3 maximum =
        points.front();

    /**
     * Проверяем каждую точку
     * по всем трём координатным осям.
     *
     * Для minimum сохраняется
     * минимальное найденное значение,
     * а для maximum — максимальное.
     */
    for (const Vec3& point : points) {
        minimum.x =
            std::min(
                minimum.x,
                point.x
            );

        minimum.y =
            std::min(
                minimum.y,
                point.y
            );

        minimum.z =
            std::min(
                minimum.z,
                point.z
            );

        maximum.x =
            std::max(
                maximum.x,
                point.x
            );

        maximum.y =
            std::max(
                maximum.y,
                point.y
            );

        maximum.z =
            std::max(
                maximum.z,
                point.z
            );
    }

    /**
     * После полного прохода:
     *
     * - minimum содержит минимальные координаты;
     * - maximum содержит максимальные координаты.
     */
    return BoundingBox{
        minimum,
        maximum
    };
}

/**
 * @brief Возвращает геометрический центр BoundingBox.
 *
 * Центр вычисляется независимо
 * по каждой координатной оси:
 *
 * center = (min + max) / 2.
 *
 * Умножение на 0.5f эквивалентно
 * делению на 2.0f.
 *
 * @return Координаты центра BoundingBox.
 */
Vec3 BoundingBox::GetCenter() const {
    return Vec3{
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    };
}

/**
 * @brief Возвращает размеры BoundingBox по трём осям.
 *
 * Размер вычисляется как разница
 * между максимальной и минимальной координатами:
 *
 * size = max - min.
 *
 * Компоненты результата соответствуют:
 *
 * - X — ширине по оси X;
 * - Y — размеру по оси Y;
 * - Z — размеру по оси Z.
 *
 * @return Размер BoundingBox по осям X, Y и Z.
 */
Vec3 BoundingBox::GetSize() const {
    return Vec3{
        max.x - min.x,
        max.y - min.y,
        max.z - min.z
    };
}