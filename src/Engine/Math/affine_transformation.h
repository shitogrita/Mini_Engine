#pragma once

#include "matrix_types.h"

#include <array>

/**
 * @brief Набор вспомогательных методов для работы
 * с аффинными преобразованиями в Mini Engine.
 *
 * AffineTransformation предоставляет статические функции
 * для создания и комбинирования матриц преобразования.
 *
 * Поддерживаются:
 *
 * - единичная матрица;
 * - translation;
 * - rotation по осям X, Y и Z;
 * - равномерный и неравномерный scale;
 * - композиция model matrix;
 * - умножение матриц;
 * - вычисление обратной аффинной матрицы;
 * - преобразование точек и направлений;
 * - преобразование Matrix4 в column-major представление.
 *
 * Matrix4 используется для переходов между системами координат,
 * в том числе между Local Space и World Space.
 *
 * Класс не хранит собственного состояния
 * и используется только через статические методы.
 */
class AffineTransformation {
public:
    /**
     * @brief Создаёт единичную матрицу 4x4.
     *
     * Единичная матрица не изменяет
     * преобразуемый вектор или другую матрицу.
     *
     * @return Единичная Matrix4.
     */
    static Matrix4 Identity4();

    /**
     * @brief Создаёт матрицу переноса.
     *
     * @param dx Смещение по оси X.
     * @param dy Смещение по оси Y.
     * @param dz Смещение по оси Z.
     *
     * @return Матрица translation.
     */
    static Matrix4 Translation4(float dx, float dy, float dz);

    /**
     * @brief Создаёт матрицу вращения вокруг оси X.
     *
     * @param degrees Угол вращения в градусах.
     *
     * @return Матрица rotation вокруг оси X.
     */
    static Matrix4 GetRotationXMatrix(float degrees);

    /**
     * @brief Создаёт матрицу вращения вокруг оси Y.
     *
     * @param degrees Угол вращения в градусах.
     *
     * @return Матрица rotation вокруг оси Y.
     */
    static Matrix4 GetRotationYMatrix(float degrees);

    /**
     * @brief Создаёт матрицу вращения вокруг оси Z.
     *
     * @param degrees Угол вращения в градусах.
     *
     * @return Матрица rotation вокруг оси Z.
     */
    static Matrix4 GetRotationZMatrix(float degrees);

    /**
     * @brief Создаёт матрицу неравномерного масштабирования.
     *
     * Каждая координатная ось получает
     * собственный коэффициент масштаба.
     *
     * @param sx Масштаб по оси X.
     * @param sy Масштаб по оси Y.
     * @param sz Масштаб по оси Z.
     *
     * @return Матрица scale.
     */
    static Matrix4 Scale(float sx, float sy, float sz);

    /**
     * @brief Создаёт матрицу равномерного масштабирования.
     *
     * Один коэффициент применяется
     * одинаково по всем трём осям.
     *
     * @param value Коэффициент масштаба.
     *
     * @return Матрица scale.
     */
    static Matrix4 Scale(float value);

    /**
     * @brief Формирует аффинную матрицу
     * из линейной части и translation.
     *
     * В качестве линейной части может использоваться
     * матрица rotation, scale
     * или их заранее сформированная комбинация.
     *
     * Translation записывается в последний столбец Matrix4.
     *
     * @param rotation_or_scale Матрица линейного преобразования.
     * @param translation Вектор переноса.
     *
     * @return Готовая аффинная Matrix4.
     */
    static Matrix4 BuildAffine4(const Matrix4& rotation_or_scale, const Vec3& translation);

    /**
     * @brief Умножает две матрицы 4x4.
     *
     * Результат соответствует:
     *
     * lhs * rhs.
     *
     * Порядок аргументов важен,
     * поскольку умножение матриц не является коммутативным.
     *
     * @param lhs Левая матрица.
     * @param rhs Правая матрица.
     *
     * @return Результат умножения lhs на rhs.
     */
    static Matrix4 Multiply4(const Matrix4& lhs, const Matrix4& rhs);

    /**
     * @brief Формирует итоговую model matrix объекта.
     *
     * Матрица строится из:
     *
     * - position;
     * - rotation;
     * - scale.
     *
     * Текущий порядок преобразований:
     *
     * M = T * Rz * Ry * Rx * S.
     *
     * @param position Позиция объекта.
     * @param rotation_degrees Углы вращения в градусах.
     * @param scale Масштаб объекта.
     *
     * @return Итоговая model matrix.
     */
    static Matrix4 ComposeModelMatrix(const Vec3& position, const Vec3& rotation_degrees, const Vec3& scale);

    /**
     * @brief Преобразует Matrix4
     * в одномерное column-major представление.
     *
     * Результат содержит 16 элементов
     * в порядке следования столбцов матрицы.
     *
     * @param matrix Исходная Matrix4.
     *
     * @return Массив из 16 float.
     */
    static std::array<float, 16> GetColMajor(const Matrix4& matrix);

    /**
     * @brief Вычисляет обратную матрицу аффинного преобразования.
     *
     * Обратная матрица выполняет преобразование
     * в противоположном направлении.
     *
     * Например:
     *
     * Model Matrix:
     *
     * Local Space -> World Space
     *
     * Inverse Model Matrix:
     *
     * World Space -> Local Space
     *
     * Для аффинной матрицы вида:
     *
     * | A T |
     * | 0 1 |
     *
     * обратная матрица имеет вид:
     *
     * | A^-1   -A^-1 * T |
     * | 0               1 |
     *
     * Если линейная часть матрицы необратима,
     * реализация использует Identity4()
     * как безопасный результат.
     *
     * @param matrix Исходная аффинная матрица.
     *
     * @return Обратная матрица преобразования.
     */
    static Matrix4 InverseAffine(const Matrix4& matrix);

    /**
     * @brief Преобразует точку при помощи Matrix4.
     *
     * Точка рассматривается как однородный вектор
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
    static Vec3 TransformPoint(const Matrix4& matrix, const Vec3& point);

    /**
     * @brief Преобразует направление при помощи Matrix4.
     *
     * Направление рассматривается как однородный вектор
     * с координатой w = 0.
     *
     * Поэтому translation на него не влияет.
     *
     * Применяется только линейная часть матрицы,
     * содержащая rotation и scale.
     *
     * @param matrix Матрица преобразования.
     * @param direction Исходное направление.
     *
     * @return Преобразованное направление.
     */
    static Vec3 TransformDirection(const Matrix4& matrix, const Vec3& direction);
};