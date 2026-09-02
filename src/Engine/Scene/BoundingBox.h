#pragma once

#include "Engine/Math/matrix_types.h"

#include <vector>


/**
 * @brief Осево-ориентированный ограничивающий параллелепипед.
 *
 * BoundingBox хранит минимальную и максимальную точки геометрии
 * по осям X, Y и Z.
 *
 * Такая коробка используется для грубых пространственных проверок,
 * например для определения того, попал ли луч мыши в объект сцены.
 *
 * Сейчас BoundingBox хранится в локальных координатах модели,
 * то есть до применения Transform объекта.
 */
struct BoundingBox {
	Vec3 min{};
	Vec3 max{};


	/**
	 * @brief Создаёт BoundingBox, содержащий все переданные точки.
	 *
	 * Функция проходит по всем вершинам и находит:
	 * - минимальное значение X, Y, Z;
	 * - максимальное значение X, Y, Z.
	 * Если список точек пуст, возвращается BoundingBox нулевого размера.
	 * @param points Набор точек геометрии.
	 * @return Ограничивающий параллелепипед для этих точек.
	 */
	static BoundingBox FromPoints(
		const std::vector<Vec3>& points
	);


	/**
	 * @brief Возвращает центр BoundingBox.
	 * Центр вычисляется как середина между min и max:
	 * center = (min + max) / 2
	 * @return Центр ограничивающего параллелепипеда.
	 */
	Vec3 GetCenter() const;


	/**
	 * @brief Возвращает размеры BoundingBox по трём осям.
	 *
	 * X — ширина.
	 * Y — высота.
	 * Z — глубина.
	 *
	 * @return Размер BoundingBox.
	 */
	Vec3 GetSize() const;
};