#include "Engine/Scene/SceneUpdateSystem.h"

#include <algorithm>
#include <memory>

SceneUpdateSystem::SceneUpdateSystem() {
    /**
     * ThreadPool создаётся один раз вместе с SceneUpdateSystem.
     *
     * Если количество workers явно не передано,
     * ThreadPool использует std::thread::hardware_concurrency().
     */
    thread_pool_ = std::make_unique<ThreadPool>();
}

void SceneUpdateSystem::Update(Scene& scene, float delta_time, ExecutionMode mode) {
    switch (mode) {
        case ExecutionMode::SingleThreaded:
            UpdateSingleThreaded(scene, delta_time);
            break;

        case ExecutionMode::MultiThreaded:
            UpdateMultiThreaded(scene, delta_time);
            break;
    }
}

void SceneUpdateSystem::SetWorkerCount(std::size_t worker_count) {
    /**
     * ThreadPool не должен работать с нулевым количеством workers.
     *
     * Если пользователь или другая система передали 0,
     * используем минимально допустимое значение.
     */
    if (worker_count == 0) {
        worker_count = 1;
    }

    /**
     * Если ThreadPool уже имеет нужное количество workers,
     * пересоздавать его нет смысла.
     */
    if (thread_pool_ && thread_pool_->GetWorkerCount() == worker_count) {
        return;
    }

    /**
     * Перед уничтожением существующего ThreadPool
     * необходимо дождаться завершения всех уже поставленных задач.
     *
     * Это гарантирует, что ни один worker не будет работать
     * с данными Scene во время пересоздания пула.
     */
    if (thread_pool_) {
        thread_pool_->WaitIdle();
    }

    /**
     * Старый ThreadPool автоматически уничтожается благодаря unique_ptr.
     *
     * Его destructor:
     *
     * stopping_ = true
     * notify_all()
     * join() всех worker-потоков.
     *
     * После этого создаётся новый ThreadPool
     * с указанным количеством workers.
     */
    thread_pool_ = std::make_unique<ThreadPool>(worker_count);
}

std::size_t SceneUpdateSystem::GetWorkerCount() const {
    if (!thread_pool_) {
        return 0;
    }

    return thread_pool_->GetWorkerCount();
}

void SceneUpdateSystem::UpdateObject(SceneObject& object, float delta_time) {
    MotionState& motion = object.GetMotionState();

    if (!motion.enabled) {
        return;
    }

    Transform& transform = object.GetTransform();

    transform.position.x += motion.linear_velocity.x * delta_time;
    transform.position.y += motion.linear_velocity.y * delta_time;
    transform.position.z += motion.linear_velocity.z * delta_time;

    transform.rotation.x += motion.angular_velocity.x * delta_time;
    transform.rotation.y += motion.angular_velocity.y * delta_time;
    transform.rotation.z += motion.angular_velocity.z * delta_time;
}

void SceneUpdateSystem::UpdateSingleThreaded(Scene& scene, float delta_time) {
    const auto& objects = scene.GetObjects();

    /**
     * SingleThreaded является baseline режимом.
     *
     * Все SceneObject обновляются последовательно
     * в текущем потоке.
     *
     * Логика обновления одного объекта находится
     * в общей функции UpdateObject().
     */
    for (const std::shared_ptr<SceneObject>& object : objects) {
        if (!object) {
            continue;
        }

        UpdateObject(*object, delta_time);
    }
}

void SceneUpdateSystem::UpdateMultiThreaded(Scene& scene, float delta_time) {
    const auto& objects = scene.GetObjects();

    const std::size_t object_count = objects.size();

    if (object_count == 0) {
        return;
    }

    /**
     * ThreadPool должен существовать на протяжении всей работы
     * SceneUpdateSystem.
     *
     * Constructor SceneUpdateSystem создаёт его автоматически,
     * поэтому сюда обычно невозможно попасть с nullptr.
     */
    if (!thread_pool_) {
        thread_pool_ = std::make_unique<ThreadPool>();
    }

    const std::size_t worker_count = thread_pool_->GetWorkerCount();

    /**
     * Количество jobs не должно превышать:
     *
     * 1. количество worker-потоков;
     * 2. количество SceneObject.
     *
     * Например:
     *
     * workers = 10
     * objects = 4
     *
     * создавать 10 jobs бессмысленно,
     * поэтому будет создано только 4.
     */
    const std::size_t job_count =
        std::min(worker_count, object_count);

    /**
     * Разбиваем SceneObjects на крупные непрерывные chunks.
     *
     * Формула выполняет целочисленный ceil:
     *
     * chunk_size =
     * ceil(object_count / job_count)
     *
     * Например:
     *
     * 25 000 objects
     * 10 jobs
     *
     * chunk_size = 2 500.
     */
    const std::size_t chunk_size =
        (object_count + job_count - 1) / job_count;

    for (std::size_t job = 0; job < job_count; ++job) {
        const std::size_t begin =
            job * chunk_size;

        const std::size_t end =
            std::min(
                begin + chunk_size,
                object_count
            );

        /**
         * Последний chunk может отсутствовать,
         * если количество объектов не делится
         * на количество jobs равномерно.
         */
        if (begin >= object_count) {
            break;
        }

        /**
         * begin, end и delta_time захватываются по значению.
         *
         * objects захватывается по ссылке,
         * поскольку коллекция SceneObject существует
         * до завершения WaitIdle().
         *
         * Во время выполнения этих jobs структура
         * scene.GetObjects() не должна изменяться.
         */
        thread_pool_->Enqueue(
            [this, &objects, begin, end, delta_time] {
                for (std::size_t index = begin; index < end; ++index) {
                    const std::shared_ptr<SceneObject>& object =
                        objects[index];

                    if (!object) {
                        continue;
                    }

                    UpdateObject(*object, delta_time);
                }
            }
        );
    }

    /**
     * Render не должен начинаться,
     * пока workers изменяют Transform объектов.
     *
     * WaitIdle является барьером между:
     *
     * CPU Scene Update
     *
     * и
     *
     * Render.
     */
    thread_pool_->WaitIdle();
}