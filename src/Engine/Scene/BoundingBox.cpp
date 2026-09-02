#include "BoundingBox.h"

#include <algorithm>


BoundingBox BoundingBox::FromPoints(
    const std::vector<Vec3>& points
)
{
    /*
     * Если геометрия не содержит ни одной точки,
     * невозможно определить реальные границы модели.
     *
     * В таком случае возвращаем BoundingBox нулевого размера:
     *
     * min = {0, 0, 0}
     * max = {0, 0, 0}
     */
    if (points.empty()) {
        return BoundingBox{};
    }


    /*
     * В качестве начальных значений min и max
     * используем первую вершину.
     *
     * Далее будем расширять границы коробки
     * при обработке остальных точек.
     */
    Vec3 minimum =
        points.front();

    Vec3 maximum =
        points.front();


    /*
     * Проходим по всем вершинам модели.
     *
     * Для каждой координаты отдельно проверяем,
     * выходит ли текущая вершина за уже найденные границы.
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


    /*
     * После прохода по всем вершинам minimum содержит
     * минимальные координаты модели,
     * а maximum — максимальные.
     */
    return BoundingBox{
        minimum,
        maximum
    };
}


Vec3 BoundingBox::GetCenter() const
{
    /*
     * Центр интервала находится как:
     *
     *     (min + max) / 2
     *
     * Умножение на 0.5f эквивалентно делению на 2.0f.
     */
    return Vec3{
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    };
}


Vec3 BoundingBox::GetSize() const
{
    /*
     * Размер по каждой оси определяется как
     * разница между максимальной и минимальной координатами.
     */
    return Vec3{
        max.x - min.x,
        max.y - min.y,
        max.z - min.z
    };
}