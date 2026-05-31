#include "affine_transformation.h"

#include <cmath>

namespace {
    constexpr float kPi = 3.14159265358979323846f;
}

Matrix4 AffineTransformation::Identity4() {
    Matrix4 m{};

    for (int i = 0; i < 4; ++i) {
        m[i][i] = 1.0f;
    }

    return m;
}

Matrix4 AffineTransformation::Translation4(float dx, float dy, float dz) {
    Matrix4 t = Identity4();

    t[0][3] = dx;
    t[1][3] = dy;
    t[2][3] = dz;

    return t;
}

Matrix4 AffineTransformation::GetRotationXMatrix(float degrees) {
    const float radians = degrees * kPi / 180.0f;
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    return {{
        {{1.0f, 0.0f, 0.0f, 0.0f}},
        {{0.0f, c,   -s,   0.0f}},
        {{0.0f, s,    c,   0.0f}},
        {{0.0f, 0.0f, 0.0f, 1.0f}}
    }};
}

Matrix4 AffineTransformation::GetRotationYMatrix(float degrees) {
    const float radians = degrees * kPi / 180.0f;
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    return {{
        {{ c,   0.0f, s,   0.0f}},
        {{0.0f, 1.0f, 0.0f, 0.0f}},
        {{-s,   0.0f, c,   0.0f}},
        {{0.0f, 0.0f, 0.0f, 1.0f}}
    }};
}

Matrix4 AffineTransformation::GetRotationZMatrix(float degrees) {
    const float radians = degrees * kPi / 180.0f;
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    return {{
        {{c,   -s,   0.0f, 0.0f}},
        {{s,    c,   0.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f, 1.0f}}
    }};
}

Matrix4 AffineTransformation::Scale(float sx, float sy, float sz) {
    Matrix4 m = Identity4();

    m[0][0] = sx;
    m[1][1] = sy;
    m[2][2] = sz;

    return m;
}

Matrix4 AffineTransformation::Scale(float value) {
    return Scale(value, value, value);
}

Matrix4 AffineTransformation::BuildAffine4(const Matrix4& rotation_or_scale,
                                           const Vec3& translation) {
    Matrix4 t = rotation_or_scale;

    t[0][3] = translation.x;
    t[1][3] = translation.y;
    t[2][3] = translation.z;

    t[3][0] = 0.0f;
    t[3][1] = 0.0f;
    t[3][2] = 0.0f;
    t[3][3] = 1.0f;

    return t;
}

Matrix4 AffineTransformation::Multiply4(const Matrix4& lhs,
                                        const Matrix4& rhs) {
    Matrix4 result{};

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;

            for (int k = 0; k < 4; ++k) {
                sum += lhs[row][k] * rhs[k][col];
            }

            result[row][col] = sum;
        }
    }

    return result;
}

Matrix4 AffineTransformation::ComposeModelMatrix(const Vec3& position,
                                                 const Vec3& rotation_degrees,
                                                 const Vec3& scale) {
    const Matrix4 t = Translation4(position.x, position.y, position.z);

    const Matrix4 rx = GetRotationXMatrix(rotation_degrees.x);
    const Matrix4 ry = GetRotationYMatrix(rotation_degrees.y);
    const Matrix4 rz = GetRotationZMatrix(rotation_degrees.z);

    const Matrix4 s = Scale(scale.x, scale.y, scale.z);

    // M = T * Rz * Ry * Rx * S
    return Multiply4(
        t,
        Multiply4(
            rz,
            Multiply4(
                ry,
                Multiply4(rx, s)
            )
        )
    );
}

std::array<float, 16> AffineTransformation::GetColMajor(const Matrix4& matrix) {
    std::array<float, 16> out{};

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[col * 4 + row] = matrix[row][col];
        }
    }

    return out;
}