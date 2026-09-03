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

Matrix4 AffineTransformation::InverseAffine (const Matrix4& matrix) {
    /*
     * Создаём матрицу, в которую будем
     * записывать обратное преобразование.
     */
    Matrix4 inverse{};


    /*
     * Берём верхнюю левую часть 3x3.
     *
     * Для аффинной матрицы именно эта часть
     * содержит rotation и scale.
     *
     * | a b c |
     * | d e f |
     * | g h i |
     */
    const float a = matrix[0][0];
    const float b = matrix[0][1];
    const float c = matrix[0][2];

    const float d = matrix[1][0];
    const float e = matrix[1][1];
    const float f = matrix[1][2];

    const float g = matrix[2][0];
    const float h = matrix[2][1];
    const float i = matrix[2][2];


    /*
     * Вычисляем определитель матрицы 3x3.
     *
     * Если determinant равен нулю,
     * обратной матрицы не существует.
     */
    const float determinant =
        a * (e * i - f * h) -
        b * (d * i - f * g) +
        c * (d * h - e * g);


    /*
     * Проверяем determinant через epsilon,
     * потому что float нельзя надёжно
     * сравнивать с нулём напрямую.
     *
     * Например, scale = (1, 0, 1)
     * делает преобразование необратимым.
     */
    if (
        std::abs(determinant) <
        0.000001f
    ) {
        /*
         * Пока используем безопасный fallback.
         *
         * Позже при необходимости можно будет
         * добавить отдельную обработку ошибки.
         */
        return Identity4();
    }


    /*
     * Строим обратную матрицу
     * для верхнего блока 3x3.
     */
    inverse[0][0] =
        (e * i - f * h) /
        determinant;

    inverse[0][1] =
        (c * h - b * i) /
        determinant;

    inverse[0][2] =
        (b * f - c * e) /
        determinant;


    inverse[1][0] =
        (f * g - d * i) /
        determinant;

    inverse[1][1] =
        (a * i - c * g) /
        determinant;

    inverse[1][2] =
        (c * d - a * f) /
        determinant;


    inverse[2][0] =
        (d * h - e * g) /
        determinant;

    inverse[2][1] =
        (b * g - a * h) /
        determinant;

    inverse[2][2] =
        (a * e - b * d) /
        determinant;


    /*
     * Извлекаем исходный translation.
     */
    const float tx =
        matrix[0][3];

    const float ty =
        matrix[1][3];

    const float tz =
        matrix[2][3];


    /*
     * Для аффинной матрицы:
     *
     * M =
     *
     * | A T |
     * | 0 1 |
     *
     * обратная матрица:
     *
     * M^-1 =
     *
     * | A^-1   -A^-1*T |
     * | 0             1 |
     *
     * Поэтому обратный translation —
     * это не просто (-tx, -ty, -tz).
     *
     * Сначала translation должен пройти
     * через обратный rotation/scale.
     */
    inverse[0][3] =
        -(
            inverse[0][0] * tx +
            inverse[0][1] * ty +
            inverse[0][2] * tz
        );

    inverse[1][3] =
        -(
            inverse[1][0] * tx +
            inverse[1][1] * ty +
            inverse[1][2] * tz
        );

    inverse[2][3] =
        -(
            inverse[2][0] * tx +
            inverse[2][1] * ty +
            inverse[2][2] * tz
        );


    /*
     * Последняя строка обычной
     * аффинной матрицы всегда:
     *
     * [0 0 0 1]
     */
    inverse[3][0] = 0.0f;
    inverse[3][1] = 0.0f;
    inverse[3][2] = 0.0f;
    inverse[3][3] = 1.0f;


    return inverse;
}

Vec3 AffineTransformation::TransformPoint (const Matrix4& matrix, const Vec3& point) {
    Vec3 result{};

    /*
     * Точка имеет однородную координату w = 1.
     *
     * Поэтому на неё влияют:
     *
     * - масштабирование;
     * - вращение;
     * - перемещение.
     */
    result.x =
        matrix[0][0] * point.x +
        matrix[0][1] * point.y +
        matrix[0][2] * point.z +
        matrix[0][3];

    result.y =
        matrix[1][0] * point.x +
        matrix[1][1] * point.y +
        matrix[1][2] * point.z +
        matrix[1][3];

    result.z =
        matrix[2][0] * point.x +
        matrix[2][1] * point.y +
        matrix[2][2] * point.z +
        matrix[2][3];

    return result;
}

Vec3 AffineTransformation::TransformDirection (const Matrix4& matrix, const Vec3& direction) {
        Vec3 result{};

        /*
         * Направление имеет w = 0,
         * поэтому translation на него не влияет.
         */
        result.x =
            matrix[0][0] * direction.x +
            matrix[0][1] * direction.y +
            matrix[0][2] * direction.z;

        result.y =
            matrix[1][0] * direction.x +
            matrix[1][1] * direction.y +
            matrix[1][2] * direction.z;

        result.z =
            matrix[2][0] * direction.x +
            matrix[2][1] * direction.y +
            matrix[2][2] * direction.z;

        return result;
}