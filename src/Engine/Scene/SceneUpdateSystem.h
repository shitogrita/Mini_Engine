#pragma once

#include "Engine/Core/ExecutionMode.h"
#include "Engine/Jobs/ThreadPool.h"
#include "Engine/Scene/Scene.h"

#include <memory>

/**
 * @brief Выполняет CPU-обновление объектов Scene.
 *
 * SceneUpdateSystem отвечает за изменение CPU-side состояния
 * объектов сцены перед последующими этапами обработки кадра.
 *
 * Система поддерживает два режима выполнения:
 *
 * ExecutionMode::SingleThreaded
 *
 * Все SceneObject обновляются последовательно
 * в потоке, вызвавшем Update().
 *
 * ExecutionMode::MultiThreaded
 *
 * Объекты Scene разделяются на несколько непрерывных диапазонов,
 * после чего эти диапазоны передаются постоянным
 * worker-потокам ThreadPool.
 *
 * В обоих режимах используется одна и та же функция UpdateObject().
 * Таким образом, логика изменения отдельного SceneObject остаётся
 * одинаковой, а отличается только стратегия выполнения вычислений.
 *
 * Это позволяет непосредственно сравнивать
 * SingleThreaded и MultiThreaded режимы.
 *
 * SceneUpdateSystem выполняет только CPU-side работу.
 * OpenGL-вызовы в этой системе отсутствуют.
 */
class SceneUpdateSystem {
public:
    /**
     * @brief Создаёт систему обновления сцены.
     *
     * Вместе с SceneUpdateSystem создаётся ThreadPool,
     * который затем переиспользуется между обновлениями.
     *
     * Если количество workers явно не задаётся,
     * используется конфигурация ThreadPool по умолчанию.
     */
    SceneUpdateSystem();

    /**
     * @brief Обновляет Scene в выбранном режиме выполнения.
     *
     * Метод является общей точкой входа
     * для CPU-обновления объектов сцены.
     *
     * В зависимости от mode управление передаётся:
     *
     * - UpdateSingleThreaded();
     * - UpdateMultiThreaded().
     *
     * @param scene Сцена, объекты которой необходимо обновить.
     * @param delta_time Время между обновлениями.
     * @param mode Режим выполнения CPU-обновления.
     */
    void Update(Scene& scene, float delta_time, ExecutionMode mode);

    /**
     * @brief Возвращает текущее количество worker-потоков.
     *
     * Значение соответствует количеству workers
     * текущего ThreadPool.
     *
     * Если ThreadPool отсутствует,
     * реализация возвращает 0.
     *
     * @return Количество worker-потоков.
     */
    std::size_t GetWorkerCount() const;

    /**
     * @brief Устанавливает количество worker-потоков.
     *
     * Если worker_count равен 0,
     * используется минимальное допустимое значение 1.
     *
     * Если указанное количество уже используется,
     * ThreadPool не пересоздаётся.
     *
     * При изменении количества workers
     * система сначала ожидает завершения текущих задач,
     * после чего создаёт новый ThreadPool.
     *
     * @param worker_count Требуемое количество worker-потоков.
     */
    void SetWorkerCount(std::size_t worker_count);

private:
    /**
     * @brief Обновляет состояние одного SceneObject.
     *
     * Эта функция содержит общую вычислительную логику,
     * используемую и в SingleThreaded,
     * и в MultiThreaded режиме.
     *
     * Если MotionState::enabled равен false,
     * состояние объекта не изменяется.
     *
     * Для активного MotionState обновляются:
     *
     * - Transform::position на основе linear_velocity;
     * - Transform::rotation на основе angular_velocity.
     *
     * Изменение выполняется с учётом delta_time.
     *
     * @param object Обновляемый SceneObject.
     * @param delta_time Время между обновлениями.
     */
    void UpdateObject(SceneObject& object, float delta_time);

    /**
     * @brief Последовательно обновляет все объекты Scene.
     *
     * SceneObject обрабатываются один за другим
     * в текущем вызывающем потоке.
     *
     * Для каждого существующего объекта
     * вызывается UpdateObject().
     *
     * nullptr-элементы контейнера Scene пропускаются.
     *
     * Этот режим используется как последовательный baseline
     * для сравнения с MultiThreaded выполнением.
     *
     * @param scene Обновляемая сцена.
     * @param delta_time Время между обновлениями.
     */
    void UpdateSingleThreaded(Scene& scene, float delta_time);

    /**
     * @brief Параллельно обновляет объекты Scene.
     *
     * Контейнер SceneObject разбивается
     * на несколько непрерывных диапазонов.
     *
     * Каждый диапазон передаётся ThreadPool
     * как отдельная задача.
     *
     * Количество jobs ограничивается:
     *
     * - количеством worker-потоков;
     * - количеством объектов сцены.
     *
     * После постановки задач вызывается ThreadPool::WaitIdle(),
     * поэтому метод возвращает управление только после завершения
     * всех jobs текущего Scene Update.
     *
     * Во время выполнения задач структура контейнера
     * Scene::GetObjects() не должна изменяться.
     *
     * @param scene Обновляемая сцена.
     * @param delta_time Время между обновлениями.
     */
    void UpdateMultiThreaded(Scene& scene, float delta_time);

    /**
     * @brief Пул постоянных worker-потоков.
     *
     * ThreadPool обычно создаётся вместе с SceneUpdateSystem
     * и переиспользуется между кадрами.
     *
     * При вызове SetWorkerCount() с новым количеством workers
     * существующий пул может быть пересоздан.
     *
     * Использование постоянных потоков позволяет избежать
     * создания и уничтожения std::thread
     * для каждого обновления сцены.
     */
    std::unique_ptr<ThreadPool> thread_pool_;
};