#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/Scene.h"

#include <memory>

/**
 * @brief Нагрузочный тест сцены из 1000 Cube.
 *
 * Cube1kTest создаёт фиксированную тестовую сцену,
 * содержащую 1000 объектов типа Cube.
 *
 * Объекты размещаются в трёхмерной сетке
 * и используют один общий Mesh.
 *
 * Для объектов включается MotionState,
 * поэтому они участвуют в CPU Scene Update.
 * В текущей конфигурации Cube имеют
 * ненулевую угловую скорость и вращаются,
 * но не перемещаются линейно.
 *
 * Тест используется для сравнения:
 *
 * - ExecutionMode::SingleThreaded;
 * - ExecutionMode::MultiThreaded.
 *
 * Благодаря одинаковому набору объектов
 * и одинаковой логике UpdateObject()
 * можно сравнивать производительность
 * двух стратегий выполнения Scene Update.
 */
class Cube1kTest {
public:
	/**
	 * @brief Создаёт тестовую сцену из 1000 Cube.
	 *
	 * Объекты располагаются сеткой:
	 *
	 * 10 x 10 x 10.
	 *
	 * Все SceneObject используют общий cube_mesh,
	 * но имеют собственные Transform и MotionState.
	 *
	 * Если cube_mesh равен nullptr,
	 * метод завершает работу без добавления объектов.
	 *
	 * @param scene Сцена, в которую добавляются тестовые Cube.
	 * @param cube_mesh Общий Mesh, используемый всеми объектами.
	 */
	static void Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh);
};