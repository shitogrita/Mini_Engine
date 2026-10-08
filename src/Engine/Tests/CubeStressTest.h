#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/Scene.h"

#include <cstddef>
#include <memory>

/**
 * @brief Генератор нагрузочной сцены из большого количества Cube.
 *
 * CubeStressTest используется для сравнения производительности
 * SingleThreaded и MultiThreaded режимов SceneUpdateSystem.
 *
 * Все SceneObject используют один общий Mesh.
 * Это важно: увеличение object_count не должно приводить
 * к созданию тысяч одинаковых GPU-буферов.
 *
 * Отличается только количество SceneObject и их Transform/MotionState.
 */
class CubeStressTest {
public:
	/**
	 * @brief Создаёт тестовую сцену с заданным количеством кубов.
	 *
	 * @param scene Сцена, в которую будут добавлены объекты.
	 * @param cube_mesh Общий Mesh куба для всех SceneObject.
	 * @param object_count Количество создаваемых объектов.
	 */
	static void Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh, std::size_t object_count);
};