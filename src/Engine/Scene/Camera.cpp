#include "Camera.h"

#include <algorithm>
#include <cmath>


namespace {

constexpr float kPi =
    3.14159265358979323846f;


float DegreesToRadians(float degrees)
{
    return degrees * kPi / 180.0f;
}


float Dot(
    const Vec3& first,
    const Vec3& second
)
{
    return
        first.x * second.x +
        first.y * second.y +
        first.z * second.z;
}


Vec3 Cross(
    const Vec3& first,
    const Vec3& second
)
{
    return Vec3{
        first.y * second.z -
            first.z * second.y,

        first.z * second.x -
            first.x * second.z,

        first.x * second.y -
            first.y * second.x
    };
}


Vec3 Normalize(const Vec3& vector)
{
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


void Camera::MoveForward(float distance)
{
    const Vec3 forward =
        GetForwardDirection();

    position_.x += forward.x * distance;
    position_.y += forward.y * distance;
    position_.z += forward.z * distance;
}


void Camera::MoveBackward(float distance)
{
    MoveForward(-distance);
}


void Camera::MoveLeft(float distance)
{
    const Vec3 right =
        GetRightDirection();

    position_.x -= right.x * distance;
    position_.y -= right.y * distance;
    position_.z -= right.z * distance;
}


void Camera::MoveRight(float distance)
{
    const Vec3 right =
        GetRightDirection();

    position_.x += right.x * distance;
    position_.y += right.y * distance;
    position_.z += right.z * distance;
}


void Camera::MoveUp(float distance)
{
    // Пока Q/E работают относительно глобальной оси Y.
    position_.y += distance;
}


void Camera::MoveDown(float distance)
{
    position_.y -= distance;
}


void Camera::Rotate(
    float yaw_delta_degrees,
    float pitch_delta_degrees
)
{
    yaw_degrees_ +=
        yaw_delta_degrees;

    pitch_degrees_ +=
        pitch_delta_degrees;

    // Не позволяем камере переворачиваться через верх.
    pitch_degrees_ =
        std::clamp(
            pitch_degrees_,
            -89.0f,
            89.0f
        );
}


Matrix4 Camera::GetViewMatrix() const
{
    const Vec3 forward =
        GetForwardDirection();

    const Vec3 world_up{
        0.0f,
        1.0f,
        0.0f
    };

    const Vec3 right =
        Normalize(
            Cross(
                forward,
                world_up
            )
        );

    const Vec3 up =
        Cross(
            right,
            forward
        );

    Matrix4 view{};

    /*
     * View matrix переводит координаты мира
     * в систему координат камеры.
     *
     * Мы используем column vectors:
     *
     * transformed = View * position
     */
    view[0][0] = right.x;
    view[0][1] = right.y;
    view[0][2] = right.z;
    view[0][3] = -Dot(right, position_);

    view[1][0] = up.x;
    view[1][1] = up.y;
    view[1][2] = up.z;
    view[1][3] = -Dot(up, position_);

    view[2][0] = -forward.x;
    view[2][1] = -forward.y;
    view[2][2] = -forward.z;
    view[2][3] = Dot(forward, position_);

    view[3][0] = 0.0f;
    view[3][1] = 0.0f;
    view[3][2] = 0.0f;
    view[3][3] = 1.0f;

    return view;
}


const Vec3& Camera::GetPosition() const
{
    return position_;
}


float Camera::GetYaw() const
{
    return yaw_degrees_;
}


float Camera::GetPitch() const
{
    return pitch_degrees_;
}


Vec3 Camera::GetForwardDirection() const
{
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


Vec3 Camera::GetRightDirection() const
{
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

Vec3 Camera::GetForward() const {
    return GetForwardDirection();
}


Vec3 Camera::GetRight() const {
    return GetRightDirection();
}


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