#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace {

/**
 * @brief Число π, используемое для перевода градусов в радианы.
 */
constexpr float kPi = 3.14159265358979323846f;

/**
 * @brief Переводит угол из градусов в радианы.
 *
 * @param degrees Угол в градусах.
 *
 * @return Угол в радианах.
 */
float DegreesToRadians(float degrees) {
    return degrees * kPi / 180.0f;
}

/**
 * @brief Вычисляет скалярное произведение двух Vec3.
 *
 * Скалярное произведение используется,
 * в частности, при построении View Matrix
 * для вычисления translation относительно позиции камеры.
 *
 * @param first Первый вектор.
 * @param second Второй вектор.
 *
 * @return Скалярное произведение first и second.
 */
float Dot(const Vec3& first, const Vec3& second) {
    return
        first.x * second.x +
        first.y * second.y +
        first.z * second.z;
}

/**
 * @brief Вычисляет векторное произведение двух Vec3.
 *
 * Результат перпендикулярен обоим исходным векторам
 * и используется для построения локального базиса камеры.
 *
 * @param first Первый вектор.
 * @param second Второй вектор.
 *
 * @return Векторное произведение first и second.
 */
Vec3 Cross(const Vec3& first, const Vec3& second) {
    return Vec3{
        first.y * second.z -
            first.z * second.y,

        first.z * second.x -
            first.x * second.z,

        first.x * second.y -
            first.y * second.x
    };
}

/**
 * @brief Возвращает нормализованную копию Vec3.
 *
 * Длина результирующего вектора равна 1,
 * если исходный вектор имеет ненулевую длину.
 *
 * Для вектора с длиной меньше или равной 0.000001f
 * возвращается Vec3{}.
 *
 * @param vector Исходный вектор.
 *
 * @return Нормализованный вектор
 * или нулевой Vec3 для практически нулевой длины.
 */
Vec3 Normalize(const Vec3& vector) {
    const float length =
        std::sqrt(
            vector.x * vector.x +
            vector.y * vector.y +
            vector.z * vector.z
        );

    if (length <= 0.000001f) {
        return Vec3{};
    }

    return Vec3{
        vector.x / length,
        vector.y / length,
        vector.z / length
    };
}

} // namespace

/**
 * @brief Перемещает камеру вперёд вдоль её текущего направления.
 *
 * Направление движения определяется через
 * GetForwardDirection() с учётом текущих yaw и pitch.
 *
 * @param distance Расстояние перемещения.
 */
void Camera::MoveForward(float distance) {
    const Vec3 forward =
        GetForwardDirection();

    position_.x += forward.x * distance;
    position_.y += forward.y * distance;
    position_.z += forward.z * distance;
}

/**
 * @brief Перемещает камеру назад.
 *
 * Метод использует MoveForward()
 * с отрицательным значением distance.
 *
 * @param distance Расстояние перемещения.
 */
void Camera::MoveBackward(float distance) {
    MoveForward(-distance);
}

/**
 * @brief Перемещает камеру влево относительно её направления.
 *
 * Правый вектор камеры вычисляется через
 * GetRightDirection(), после чего вычитается
 * из текущей позиции.
 *
 * @param distance Расстояние перемещения.
 */
void Camera::MoveLeft(float distance) {
    const Vec3 right =
        GetRightDirection();

    position_.x -= right.x * distance;
    position_.y -= right.y * distance;
    position_.z -= right.z * distance;
}

/**
 * @brief Перемещает камеру вправо относительно её направления.
 *
 * @param distance Расстояние перемещения.
 */
void Camera::MoveRight(float distance) {
    const Vec3 right =
        GetRightDirection();

    position_.x += right.x * distance;
    position_.y += right.y * distance;
    position_.z += right.z * distance;
}

/**
 * @brief Перемещает камеру вверх вдоль глобальной оси Y.
 *
 * В текущей реализации вертикальное движение
 * не зависит от локального наклона камеры.
 *
 * @param distance Расстояние перемещения.
 */
void Camera::MoveUp(float distance) {
    // Пока Q/E работают относительно глобальной оси Y.
    position_.y += distance;
}

/**
 * @brief Перемещает камеру вниз вдоль глобальной оси Y.
 *
 * @param distance Расстояние перемещения.
 */
void Camera::MoveDown(float distance) {
    position_.y -= distance;
}

/**
 * @brief Изменяет ориентацию камеры.
 *
 * yaw управляет горизонтальным вращением,
 * а pitch — вертикальным.
 *
 * После изменения pitch ограничивается диапазоном:
 *
 * [-89°, 89°]
 *
 * Это предотвращает прохождение камеры
 * через строго вертикальное направление
 * и её переворот через верх или низ.
 *
 * @param yaw_delta_degrees Изменение yaw в градусах.
 * @param pitch_delta_degrees Изменение pitch в градусах.
 */
void Camera::Rotate(float yaw_delta_degrees, float pitch_delta_degrees) {
    yaw_degrees_ +=
        yaw_delta_degrees;

    pitch_degrees_ +=
        pitch_delta_degrees;

    /**
     * Ограничиваем вертикальный угол,
     * чтобы камера не переворачивалась.
     */
    pitch_degrees_ =
        std::clamp(
            pitch_degrees_,
            -89.0f,
            89.0f
        );
}

/**
 * @brief Формирует View Matrix камеры.
 *
 * Сначала вычисляется ортонормированный базис камеры:
 *
 * - forward — направление взгляда;
 * - right — направление вправо;
 * - up — направление вверх.
 *
 * Для построения right используется глобальный up-вектор:
 *
 * (0, 1, 0).
 *
 * View Matrix переводит координаты:
 *
 * World Space -> Camera/View Space.
 *
 * В текущем математическом соглашении
 * используются column vectors:
 *
 * transformed = View * position.
 *
 * @return View Matrix камеры.
 */
Matrix4 Camera::GetViewMatrix() const {
    const Vec3 forward =
        GetForwardDirection();

    const Vec3 world_up{
        0.0f,
        1.0f,
        0.0f
    };

    /**
     * Right-вектор вычисляется как:
     *
     * forward x world_up.
     */
    const Vec3 right =
        Normalize(
            Cross(
                forward,
                world_up
            )
        );

    /**
     * Up-вектор восстанавливается
     * из уже вычисленных right и forward.
     */
    const Vec3 up =
        Cross(
            right,
            forward
        );

    Matrix4 view{};

    /**
     * Первая строка задаёт ось Right камеры.
     *
     * Translation вычисляется как
     * отрицательная проекция позиции камеры
     * на соответствующую ось.
     */
    view[0][0] = right.x;
    view[0][1] = right.y;
    view[0][2] = right.z;
    view[0][3] = -Dot(right, position_);

    /**
     * Вторая строка задаёт ось Up камеры.
     */
    view[1][0] = up.x;
    view[1][1] = up.y;
    view[1][2] = up.z;
    view[1][3] = -Dot(up, position_);

    /**
     * Третья строка использует отрицательное
     * направление Forward.
     */
    view[2][0] = -forward.x;
    view[2][1] = -forward.y;
    view[2][2] = -forward.z;
    view[2][3] = Dot(forward, position_);

    /**
     * Последняя строка сохраняет
     * стандартную форму аффинной матрицы.
     */
    view[3][0] = 0.0f;
    view[3][1] = 0.0f;
    view[3][2] = 0.0f;
    view[3][3] = 1.0f;

    return view;
}

/**
 * @brief Возвращает текущую позицию камеры.
 *
 * @return Константная ссылка на position_.
 */
const Vec3& Camera::GetPosition() const {
    return position_;
}

/**
 * @brief Возвращает горизонтальный угол камеры.
 *
 * @return Текущее значение yaw в градусах.
 */
float Camera::GetYaw() const {
    return yaw_degrees_;
}

/**
 * @brief Возвращает вертикальный угол камеры.
 *
 * @return Текущее значение pitch в градусах.
 */
float Camera::GetPitch() const {
    return pitch_degrees_;
}

/**
 * @brief Вычисляет нормализованное направление взгляда камеры.
 *
 * yaw и pitch предварительно переводятся
 * из градусов в радианы.
 *
 * Направление рассчитывается по сферическим координатам:
 *
 * X = cos(yaw) * cos(pitch)
 *
 * Y = sin(pitch)
 *
 * Z = sin(yaw) * cos(pitch)
 *
 * После вычисления результат нормализуется.
 *
 * @return Нормализованный forward-вектор камеры.
 */
Vec3 Camera::GetForwardDirection() const {
    const float yaw =
        DegreesToRadians(
            yaw_degrees_
        );

    const float pitch =
        DegreesToRadians(
            pitch_degrees_
        );

    Vec3 forward{
        std::cos(yaw) *
            std::cos(pitch),

        std::sin(pitch),

        std::sin(yaw) *
            std::cos(pitch)
    };

    return Normalize(forward);
}

/**
 * @brief Вычисляет направление вправо относительно камеры.
 *
 * Вектор получается через векторное произведение:
 *
 * forward x world_up.
 *
 * После вычисления результат нормализуется.
 *
 * @return Нормализованный right-вектор камеры.
 */
Vec3 Camera::GetRightDirection() const {
    const Vec3 world_up{
        0.0f,
        1.0f,
        0.0f
    };

    return Normalize(
        Cross(
            GetForwardDirection(),
            world_up
        )
    );
}

/**
 * @brief Возвращает forward-вектор камеры.
 *
 * Метод является обёрткой над GetForwardDirection().
 *
 * @return Направление взгляда камеры.
 */
Vec3 Camera::GetForward() const {
    return GetForwardDirection();
}

/**
 * @brief Возвращает right-вектор камеры.
 *
 * Метод является обёрткой над GetRightDirection().
 *
 * @return Направление вправо относительно камеры.
 */
Vec3 Camera::GetRight() const {
    return GetRightDirection();
}

/**
 * @brief Возвращает локальный up-вектор камеры.
 *
 * Up вычисляется на основе текущих
 * right и forward:
 *
 * up = right x forward.
 *
 * После вычисления результат нормализуется.
 *
 * @return Нормализованный up-вектор камеры.
 */
Vec3 Camera::GetUp() const {
    const Vec3 right =
        GetRightDirection();

    const Vec3 forward =
        GetForwardDirection();

    return Normalize(
        Cross(
            right,
            forward
        )
    );
}

/**
 * @brief Устанавливает позицию камеры.
 *
 * @param position Новая позиция камеры.
 */
void Camera::SetPosition(const Vec3& position) {
    position_ = position;
}