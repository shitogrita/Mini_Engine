#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/Scene.h"

#include <memory>

/**
 * @brief Нагрузочный тест сцены из 1000 движущихся Cube.
 *
 * Тест используется для сравнения SingleThreaded и MultiThreaded
 * режимов обновления сцены.
 */
class Cube1kTest {
public:
	static void Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh);
};