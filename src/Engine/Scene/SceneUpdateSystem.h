#pragma once

#include "Engine/Core/ExecutionMode.h"
#include "Engine/Scene/Scene.h"

/**
 * @brief Система обновления объектов сцены.
 *
 * SceneUpdateSystem отвечает за CPU-обновление Scene.
 * Способ выполнения зависит от выбранного ExecutionMode.
 *
 * Рендеринг и OpenGL-вызовы здесь не выполняются.
 */
class SceneUpdateSystem {
public:
	/**
	 * @brief Обновляет сцену в выбранном режиме.
	 *
	 * @param scene Обновляемая сцена.
	 * @param delta_time Время между кадрами в секундах.
	 * @param mode Режим выполнения.
	 */
	void Update(Scene& scene, float delta_time, ExecutionMode mode);

private:
	/**
	 * @brief Однопоточное обновление сцены.
	 */
	void UpdateSingleThreaded(Scene& scene, float delta_time);

	/**
	 * @brief Многопоточное обновление сцены.
	 *
	 * Реальная многопоточная реализация будет добавлена позже.
	 */
	void UpdateMultiThreaded(Scene& scene, float delta_time);
};