#include "Ray.h"

#include "../Scene/BoundingBox.h"

#include <algorithm>
#include <cmath>
#include <limits>


Vec3 Ray::GetPoint(float distance) const {

    /*
     * Вычисляем точку на луче по формуле:
     *
     * P(t) = origin + direction * t
     *
     * origin    — начало луча;
     * direction — направление луча;
     * distance  — расстояние вдоль направления.
     */

    Vec3 point{};
    point.x = origin.x + direction.x * distance;
    point.y = origin.y + direction.y * distance;
    point.z = origin.z + direction.z * distance;

    return point;
}


bool Ray::Intersects(const BoundingBox& box, float& distance) const {

    /*
     * epsilon — очень маленькое число.
     *
     * Оно используется вместо прямого сравнения float с нулём,
     * потому что числа с плавающей точкой могут содержать
     * небольшую погрешность.
     */
    constexpr float epsilon = 0.000001f;

    /*
     * В начале считаем, что возможный интервал пересечения
     * луча бесконечный:
     *
     * [-∞, +∞]
     *
     * После проверки каждой оси X, Y и Z
     * этот интервал будет постепенно сужаться.
     */
    float t_near = -std::numeric_limits<float>::infinity();
    float t_far = std::numeric_limits<float>::infinity();


    /*
     * Проверка пересечения по оси X.
     */
    if (std::abs(direction.x) < epsilon) {

        /*
         * Если луч практически не движется по X,
         * его X-координата всегда равна origin.x.
         *
         * Если origin.x находится за пределами BoundingBox,
         * луч никогда не сможет пересечь коробку.
         */
        if (origin.x < box.min.x || origin.x > box.max.x) {
            return false;
        }

    } else {

        /*
         * Находим расстояния до двух X-плоскостей BoundingBox.
         */
        float t1x = (box.min.x - origin.x) / direction.x;
        float t2x = (box.max.x - origin.x) / direction.x;

        /*
         * Если direction.x отрицательный,
         * t1x может оказаться больше t2x.
         *
         * Поэтому отдельно определяем ближайшее
         * и дальнее пересечение.
         */
        float near_x = std::min(t1x, t2x);
        float far_x = std::max(t1x, t2x);

        /*
         * Сужаем общий интервал пересечения.
         */
        t_near = std::max(t_near, near_x);
        t_far = std::min(t_far, far_x);
    }


    /*
     * Проверка пересечения по оси Y.
     */
    if (std::abs(direction.y) < epsilon) {

        /*
         * Луч не движется по Y.
         *
         * Если origin.y находится вне диапазона BoundingBox,
         * пересечения быть не может.
         */
        if (origin.y < box.min.y || origin.y > box.max.y) {
            return false;
        }

    } else {

        /*
         * Находим расстояния до двух Y-плоскостей BoundingBox.
         */
        float t1y = (box.min.y - origin.y) / direction.y;
        float t2y = (box.max.y - origin.y) / direction.y;

        float near_y = std::min(t1y, t2y);
        float far_y = std::max(t1y, t2y);

        /*
         * Ещё раз сужаем общий интервал.
         */
        t_near = std::max(t_near, near_y);
        t_far = std::min(t_far, far_y);
    }


    /*
     * Проверка пересечения по оси Z.
     */
    if (std::abs(direction.z) < epsilon) {

        /*
         * Луч не движется по Z.
         *
         * Если origin.z находится вне BoundingBox,
         * луч не сможет попасть в коробку.
         */
        if (origin.z < box.min.z || origin.z > box.max.z) {
            return false;
        }

    } else {

        /*
         * Находим расстояния до двух Z-плоскостей BoundingBox.
         */
        float t1z = (box.min.z - origin.z) / direction.z;
        float t2z = (box.max.z - origin.z) / direction.z;

        float near_z = std::min(t1z, t2z);
        float far_z = std::max(t1z, t2z);

        /*
         * Получаем окончательный общий интервал
         * пересечения по X, Y и Z.
         */
        t_near = std::max(t_near, near_z);
        t_far = std::min(t_far, far_z);
    }

    /*
     * Если ближайшая точка входа находится дальше,
     * чем ближайшая точка выхода,
     * общего интервала пересечения нет.
     */
    if (t_near > t_far) {
        return false;
    }

    /*
     * Если даже дальняя точка пересечения находится
     * позади начала луча, BoundingBox расположен
     * позади камеры.
     */
    if (t_far < 0.0f) {
        return false;
    }

    /*
     * Если t_near положительный,
     * луч входит в BoundingBox перед камерой.
     *
     * Если t_near отрицательный, значит начало луча
     * уже находится внутри BoundingBox.
     * Тогда ближайшая точка пересечения впереди — t_far.
     */
    if (t_near >= 0.0f) {
        distance = t_near;
    } else {
        distance = t_far;
    }

    return true;
}