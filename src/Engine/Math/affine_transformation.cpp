#include "affine_transformation.h"

#include <cmath>

namespace {
    /**
     * @brief Число π, используемое для перевода градусов в радианы.
     */
    constexpr float kPi = 3.14159265358979323846f;
}

/**
 * @brief Создаёт единичную матрицу 4x4.
 *
 * Единичная матрица является нейтральным элементом
 * матричного умножения:
 *
 * I * M = M
 * M * I = M
 *
 * Она используется как базовое преобразование
 * при построении матриц translation и scale.
 *
 * @return Единичная Matrix4.
 */
Matrix4 AffineTransformation::Identity4() {
    Matrix4 m{};

    for (int i = 0; i < 4; ++i) {
        m[i][i] = 1.0f;
    }

    return m;
}

/**
 * @brief Создаёт матрицу переноса.
 *
 * Значения dx, dy и dz записываются
 * в последний столбец аффинной матрицы.
 *
 * Матрица имеет вид:
 *
 * | 1 0 0 dx |
 * | 0 1 0 dy |
 * | 0 0 1 dz |
 * | 0 0 0  1 |
 *
 * @param dx Смещение по оси X.
 * @param dy Смещение по оси Y.
 * @param dz Смещение по оси Z.
 *
 * @return Матрица переноса.
 */
Matrix4 AffineTransformation::Translation4(float dx, float dy, float dz) {
    Matrix4 t = Identity4();

    t[0][3] = dx;
    t[1][3] = dy;
    t[2][3] = dz;

    return t;
}

/**
 * @brief Создаёт матрицу вращения вокруг оси X.
 *
 * Угол передаётся в градусах
 * и перед вычислением sin/cos переводится в радианы.
 *
 * @param degrees Угол вращения в градусах.
 *
 * @return Матрица вращения вокруг оси X.
 */
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

/**
 * @brief Создаёт матрицу вращения вокруг оси Y.
 *
 * Угол передаётся в градусах
 * и переводится в радианы перед вычислением.
 *
 * @param degrees Угол вращения в градусах.
 *
 * @return Матрица вращения вокруг оси Y.
 */
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

/**
 * @brief Создаёт матрицу вращения вокруг оси Z.
 *
 * Угол передаётся в градусах
 * и переводится в радианы перед вычислением.
 *
 * @param degrees Угол вращения в градусах.
 *
 * @return Матрица вращения вокруг оси Z.
 */
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

/**
 * @brief Создаёт матрицу неравномерного масштабирования.
 *
 * Каждая координатная ось получает
 * собственный коэффициент масштабирования.
 *
 * @param sx Масштаб по оси X.
 * @param sy Масштаб по оси Y.
 * @param sz Масштаб по оси Z.
 *
 * @return Матрица масштабирования.
 */
Matrix4 AffineTransformation::Scale(float sx, float sy, float sz) {
    Matrix4 m = Identity4();

    m[0][0] = sx;
    m[1][1] = sy;
    m[2][2] = sz;

    return m;
}

/**
 * @brief Создаёт матрицу равномерного масштабирования.
 *
 * Один коэффициент применяется одинаково
 * ко всем трём координатным осям.
 *
 * @param value Общий коэффициент масштабирования.
 *
 * @return Матрица равномерного масштабирования.
 */
Matrix4 AffineTransformation::Scale(float value) {
    return Scale(value, value, value);
}

/**
 * @brief Добавляет translation к матрице rotation или scale.
 *
 * Переданная Matrix4 копируется,
 * после чего её последний столбец заполняется
 * координатами translation.
 *
 * Последняя строка явно приводится к стандартному
 * виду аффинной матрицы:
 *
 * [0 0 0 1]
 *
 * @param rotation_or_scale Исходная матрица rotation или scale.
 * @param translation Вектор переноса.
 *
 * @return Готовая аффинная Matrix4.
 */
Matrix4 AffineTransformation::BuildAffine4(const Matrix4& rotation_or_scale, const Vec3& translation) {
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

/**
 * @brief Выполняет умножение двух матриц 4x4.
 *
 * Каждый элемент результирующей матрицы вычисляется
 * как скалярное произведение соответствующей строки lhs
 * и столбца rhs.
 *
 * @param lhs Левая матрица.
 * @param rhs Правая матрица.
 *
 * @return Результат lhs * rhs.
 */
Matrix4 AffineTransformation::Multiply4(const Matrix4& lhs, const Matrix4& rhs) {
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

/**
 * @brief Формирует итоговую model matrix объекта.
 *
 * Матрица собирается из:
 *
 * - position;
 * - rotation по X, Y и Z;
 * - scale.
 *
 * Порядок преобразований:
 *
 * M = T * Rz * Ry * Rx * S
 *
 * Для вектора это означает, что фактически
 * сначала применяется scale,
 * затем вращения X, Y, Z,
 * после чего translation.
 *
 * @param position Позиция объекта.
 * @param rotation_degrees Углы вращения в градусах.
 * @param scale Масштаб объекта.
 *
 * @return Итоговая model matrix.
 */
Matrix4 AffineTransformation::ComposeModelMatrix(const Vec3& position, const Vec3& rotation_degrees, const Vec3& scale) {
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

/**
 * @brief Преобразует Matrix4 в массив column-major.
 *
 * Внутреннее представление Matrix4 индексируется
 * как matrix[row][column].
 *
 * Метод переставляет элементы таким образом,
 * чтобы результат был записан в column-major порядке.
 *
 * @param matrix Исходная Matrix4.
 *
 * @return Массив из 16 float в column-major формате.
 */
std::array<float, 16> AffineTransformation::GetColMajor(const Matrix4& matrix) {
    std::array<float, 16> out{};

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[col * 4 + row] = matrix[row][col];
        }
    }

    return out;
}

/**
 * @brief Вычисляет обратную аффинную матрицу.
 *
 * Исходная матрица рассматривается в виде:
 *
 * | A T |
 * | 0 1 |
 *
 * где:
 *
 * - A — верхний блок 3x3, содержащий rotation и scale;
 * - T — translation.
 *
 * Обратная матрица вычисляется как:
 *
 * | A^-1   -A^-1 * T |
 * | 0              1 |
 *
 * Сначала вычисляется обратная матрица для блока 3x3,
 * после чего рассчитывается обратный translation.
 *
 * Если determinant блока 3x3 близок к нулю,
 * преобразование считается необратимым
 * и возвращается Identity4().
 *
 * @param matrix Исходная аффинная Matrix4.
 *
 * @return Обратная аффинная матрица
 * или Identity4() для необратимого преобразования.
 */
Matrix4 AffineTransformation::InverseAffine(const Matrix4& matrix) {
    /**
     * Матрица для результата обратного преобразования.
     */
    Matrix4 inverse{};

    /**
     * Верхний левый блок 3x3 содержит
     * линейную часть аффинного преобразования:
     * rotation и scale.
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

    /**
     * Определитель блока 3x3.
     *
     * Нулевой determinant означает,
     * что обратная матрица не существует.
     */
    const float determinant =
        a * (e * i - f * h) -
        b * (d * i - f * g) +
        c * (d * h - e * g);

    /**
     * Используется epsilon-проверка,
     * потому что значения float не следует
     * сравнивать с нулём напрямую.
     *
     * Например, scale = (1, 0, 1)
     * делает преобразование необратимым.
     */
    if (std::abs(determinant) < 0.000001f) {
        return Identity4();
    }

    /**
     * Вычисляем обратную матрицу
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

    /**
     * Translation исходной матрицы.
     */
    const float tx = matrix[0][3];
    const float ty = matrix[1][3];
    const float tz = matrix[2][3];

    /**
     * Для аффинного преобразования обратный translation:
     *
     * -A^-1 * T
     *
     * Поэтому простого изменения знака
     * исходных tx, ty и tz недостаточно.
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

    /**
     * Последняя строка стандартной
     * аффинной матрицы.
     */
    inverse[3][0] = 0.0f;
    inverse[3][1] = 0.0f;
    inverse[3][2] = 0.0f;
    inverse[3][3] = 1.0f;

    return inverse;
}

/**
 * @brief Применяет матрицу преобразования к точке.
 *
 * Точка интерпретируется как однородный вектор
 * с координатой w = 1.
 *
 * Поэтому на неё влияют:
 *
 * - scale;
 * - rotation;
 * - translation.
 *
 * @param matrix Матрица преобразования.
 * @param point Исходная точка.
 *
 * @return Преобразованная точка.
 */
Vec3 AffineTransformation::TransformPoint(const Matrix4& matrix, const Vec3& point) {
    Vec3 result{};

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

/**
 * @brief Применяет матрицу преобразования к направлению.
 *
 * Направление интерпретируется как однородный вектор
 * с координатой w = 0.
 *
 * Поэтому translation на направление не влияет.
 *
 * Применяется только верхний блок 3x3 матрицы,
 * содержащий rotation и scale.
 *
 * @param matrix Матрица преобразования.
 * @param direction Исходный вектор направления.
 *
 * @return Преобразованное направление.
 */
Vec3 AffineTransformation::TransformDirection(const Matrix4& matrix, const Vec3& direction) {
    Vec3 result{};

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