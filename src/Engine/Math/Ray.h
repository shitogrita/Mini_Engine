#pragma once

#include "matrix_types.h"

struct BoundingBox;


/**
 * @brief Представляет луч в трёхмерном пространстве.
 *
 * Луч задаётся начальной точкой и направлением.
 * Используется для пространственных запросов,
 * например для выбора объектов в сцене.
 */
struct Ray {
	Vec3 origin{};
	Vec3 direction{};

	/**
	 * @brief Возвращает точку на луче на заданном расстоянии.
	 * Точка вычисляется по формуле:
	 * P(t) = origin + direction * t
	 *
	 * @param distance Расстояние вдоль направления луча.
	 * @return Точка на луче.
	 */
	Vec3 GetPoint(float distance) const;

	/**
	 * @brief Проверяет пересечение луча с BoundingBox.
	 *
	 * @param box Проверяемый BoundingBox.
	 * @param distance Расстояние до ближайшей точки пересечения.
	 * @return true, если луч пересекает BoundingBox.
	 */
	bool Intersects(const BoundingBox& box, float& distance) const;
};