#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/Scene.h"

#include <cstddef>
#include <memory>

/**
 * @brief Генерирует нагрузочную сцену
 * из заданного количества Cube.
 *
 * CubeStressTest используется для исследования
 * производительности SceneUpdateSystem
 * при увеличении количества объектов сцены.
 *
 * Тест позволяет сравнивать:
 *
 * - ExecutionMode::SingleThreaded;
 * - ExecutionMode::MultiThreaded.
 *
 * Все создаваемые SceneObject используют
 * один общий Mesh, переданный через cube_mesh.
 *
 * Благодаря этому увеличение object_count
 * увеличивает количество объектов сцены
 * и их CPU-side состояния,
 * но не требует создания отдельного Mesh
 * для каждого Cube.
 *
 * Для каждого объекта создаются собственные:
 *
 * - Transform;
 * - MotionState;
 * - имя SceneObject.
 *
 * В текущей реализации объекты размещаются
 * в двумерной сетке и получают
 * ненулевую угловую скорость,
 * поэтому их Transform изменяется
 * при каждом Scene Update.
 */
class CubeStressTest {
public:
	/**
	 * @brief Создаёт нагрузочную сцену
	 * с заданным количеством Cube.
	 *
	 * Все объекты используют общий cube_mesh.
	 *
	 * Каждый SceneObject получает собственные
	 * Transform и MotionState.
	 *
	 * Если cube_mesh равен nullptr
	 * или object_count равен 0,
	 * метод завершает работу
	 * без добавления объектов в Scene.
	 *
	 * @param scene Сцена,
	 * в которую добавляются тестовые объекты.
	 * @param cube_mesh Общий Mesh Cube
	 * для всех создаваемых SceneObject.
	 * @param object_count Количество объектов,
	 * которое необходимо создать.
	 */
	static void Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh, std::size_t object_count);
};