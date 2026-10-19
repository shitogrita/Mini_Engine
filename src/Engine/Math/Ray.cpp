#include "Ray.h"

#include "../Scene/BoundingBox.h"

#include <algorithm>
#include <cmath>
#include <limits>

/**
 * @brief Возвращает точку на луче на заданном расстоянии от его начала.
 *
 * Точка вычисляется по параметрическому уравнению луча:
 *
 * P(t) = origin + direction * t
 *
 * где:
 *
 * - origin — начало луча;
 * - direction — направление луча;
 * - distance — параметр перемещения вдоль луча.
 *
 * @param distance Расстояние вдоль направления луча.
 *
 * @return Точка на луче.
 */
Vec3 Ray::GetPoint(float distance) const {
    Vec3 point{};

    point.x = origin.x + direction.x * distance;
    point.y = origin.y + direction.y * distance;
    point.z = origin.z + direction.z * distance;

    return point;
}

/**
 * @brief Проверяет пересечение луча с осево-ориентированным BoundingBox.
 *
 * Для проверки используется slab-подход.
 *
 * BoundingBox рассматривается как пересечение трёх диапазонов:
 *
 * - диапазона по оси X;
 * - диапазона по оси Y;
 * - диапазона по оси Z.
 *
 * Для каждой оси вычисляется интервал значений параметра t,
 * при которых луч находится внутри соответствующего диапазона.
 *
 * После этого интервалы по всем трём осям пересекаются.
 *
 * Если после обработки всех осей существует общий интервал,
 * луч пересекает BoundingBox.
 *
 * Луч задаётся уравнением:
 *
 * P(t) = origin + direction * t
 *
 * При положительном t точка находится перед началом луча.
 * При отрицательном t — позади него.
 *
 * @param box BoundingBox, с которым проверяется пересечение.
 * @param distance Расстояние до ближайшей точки пересечения
 * в положительном направлении луча.
 *
 * @return true, если луч пересекает BoundingBox;
 * иначе false.
 */
bool Ray::Intersects(const BoundingBox& box, float& distance) const {
    /**
     * Малое значение для проверки компоненты direction
     * на практически нулевое значение.
     *
     * Прямое сравнение float с 0.0f нежелательно
     * из-за возможной погрешности вычислений.
     */
    constexpr float epsilon = 0.000001f;

    /**
     * Общий допустимый интервал параметра t.
     *
     * Изначально луч считается допустимым
     * на всём диапазоне:
     *
     * [-infinity, +infinity]
     *
     * После обработки каждой координатной оси
     * этот диапазон последовательно сужается.
     */
    float t_near = -std::numeric_limits<float>::infinity();
    float t_far = std::numeric_limits<float>::infinity();

    /**
     * Проверка пересечения по оси X.
     */
    if (std::abs(direction.x) < epsilon) {
        /**
         * Если X-компонента направления практически равна нулю,
         * луч параллелен X-плоскостям BoundingBox.
         *
         * В этом случае координата origin.x
         * остаётся постоянной вдоль всего луча.
         *
         * Если начало луча находится вне диапазона
         * [box.min.x, box.max.x],
         * пересечение невозможно.
         */
        if (origin.x < box.min.x || origin.x > box.max.x) {
            return false;
        }
    } else {
        /**
         * Значения t для пересечения
         * с минимальной и максимальной X-плоскостью.
         */
        float t1x = (box.min.x - origin.x) / direction.x;
        float t2x = (box.max.x - origin.x) / direction.x;

        /**
         * При отрицательном направлении t1x и t2x
         * могут располагаться в обратном порядке.
         *
         * Поэтому отдельно определяются
         * ближнее и дальнее значения.
         */
        float near_x = std::min(t1x, t2x);
        float far_x = std::max(t1x, t2x);

        /**
         * Пересекаем текущий общий интервал
         * с допустимым интервалом по X.
         */
        t_near = std::max(t_near, near_x);
        t_far = std::min(t_far, far_x);
    }

    /**
     * Проверка пересечения по оси Y.
     */
    if (std::abs(direction.y) < epsilon) {
        /**
         * При практически нулевой Y-компоненте
         * луч параллелен Y-плоскостям BoundingBox.
         *
         * Если origin.y находится вне диапазона box,
         * пересечение невозможно.
         */
        if (origin.y < box.min.y || origin.y > box.max.y) {
            return false;
        }
    } else {
        /**
         * Значения t для двух Y-плоскостей BoundingBox.
         */
        float t1y = (box.min.y - origin.y) / direction.y;
        float t2y = (box.max.y - origin.y) / direction.y;

        float near_y = std::min(t1y, t2y);
        float far_y = std::max(t1y, t2y);

        /**
         * Сужаем общий интервал пересечения
         * с учётом диапазона по Y.
         */
        t_near = std::max(t_near, near_y);
        t_far = std::min(t_far, far_y);
    }

    /**
     * Проверка пересечения по оси Z.
     */
    if (std::abs(direction.z) < epsilon) {
        /**
         * При практически нулевой Z-компоненте
         * луч параллелен Z-плоскостям BoundingBox.
         *
         * Если origin.z находится вне диапазона,
         * пересечение невозможно.
         */
        if (origin.z < box.min.z || origin.z > box.max.z) {
            return false;
        }
    } else {
        /**
         * Значения t для двух Z-плоскостей BoundingBox.
         */
        float t1z = (box.min.z - origin.z) / direction.z;
        float t2z = (box.max.z - origin.z) / direction.z;

        float near_z = std::min(t1z, t2z);
        float far_z = std::max(t1z, t2z);

        /**
         * После обработки Z получаем окончательный
         * общий интервал пересечения по трём осям.
         */
        t_near = std::max(t_near, near_z);
        t_far = std::min(t_far, far_z);
    }

    /**
     * Если ближайшая возможная точка входа
     * находится дальше точки выхода,
     * общего интервала пересечения не существует.
     */
    if (t_near > t_far) {
        return false;
    }

    /**
     * Если даже дальняя точка пересечения
     * имеет отрицательный параметр t,
     * весь BoundingBox находится позади начала луча.
     */
    if (t_far < 0.0f) {
        return false;
    }

    /**
     * Если t_near положительный,
     * это ближайшая точка входа луча в BoundingBox.
     *
     * Если t_near отрицательный,
     * начало луча уже находится внутри BoundingBox.
     *
     * В таком случае ближайшее пересечение
     * в положительном направлении находится в t_far.
     */
    if (t_near >= 0.0f) {
        distance = t_near;
    } else {
        distance = t_far;
    }

    return true;
}