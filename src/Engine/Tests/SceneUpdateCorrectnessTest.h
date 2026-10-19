#pragma once

/**
 * @brief Проверяет корректность MultiThreaded Scene Update.
 *
 * SceneUpdateCorrectnessTest сравнивает
 * результат обновления одинаковых сцен
 * в двух режимах:
 *
 * - ExecutionMode::SingleThreaded;
 * - ExecutionMode::MultiThreaded.
 *
 * Тест предназначен не для измерения производительности,
 * а для проверки функциональной эквивалентности
 * двух стратегий выполнения SceneUpdateSystem.
 *
 * Успешное прохождение означает,
 * что после серии одинаковых обновлений
 * итоговые Transform объектов совпадают
 * в пределах заданной погрешности.
 */
class SceneUpdateCorrectnessTest {
public:
	/**
	 * @brief Запускает тест корректности Scene Update.
	 *
	 * Внутри создаются две независимые,
	 * но одинаково инициализированные Scene.
	 *
	 * Одна сцена обновляется в SingleThreaded режиме,
	 * вторая — в MultiThreaded.
	 *
	 * После выполнения серии кадров
	 * результаты сравниваются.
	 *
	 * @return true, если итоговое состояние сцен совпадает.
	 * @return false, если обнаружено различие.
	 */
	static bool Run();
};