#pragma once

#include "Engine/Assets/ImportedMeshData.h"


/**
 * @brief Генератор встроенных геометрических примитивов.
 *
 * PrimitiveGenerator создаёт геометрию примитивов
 * непосредственно в памяти без загрузки модели из файла.
 *
 * Результат имеет тот же формат ImportedMeshData,
 * который используется ObjParser.
 */
class PrimitiveGenerator {
public:

	/**
	 * @brief Создаёт куб с центром в начале координат.
	 *
	 * @param size Длина стороны куба.
	 */
	static ImportedMeshData CreateCube(
		float size = 1.0f
	);


	/**
	 * @brief Создаёт плоскость XZ
	 * с центром в начале координат.
	 */
	static ImportedMeshData CreatePlane(
		float size = 1.0f
	);


	/**
	 * @brief Создаёт сферу с заданным радиусом
	 * и детализацией.
	 */
	static ImportedMeshData CreateSphere(
		float radius = 0.5f,
		unsigned int segments = 32,
		unsigned int rings = 16
	);
};