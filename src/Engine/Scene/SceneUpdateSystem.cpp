#include "Engine/Scene/SceneUpdateSystem.h"

#include <algorithm>
#include <cstddef>
#include <memory>

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

void SceneUpdateSystem::UpdateObject(SceneObject& object, float delta_time) {
    MotionState& motion = object.GetMotionState();

    // MotionState позволяет исключить неподвижные объекты
    // из вычислительной части обновления сцены.
    if (!motion.enabled) {
        return;
    }

    Transform& transform = object.GetTransform();

    // Линейное движение.
    // velocity хранится в единицах в секунду,
    // поэтому умножаем её на время текущего кадра.
    transform.position.x += motion.linear_velocity.x * delta_time;
    transform.position.y += motion.linear_velocity.y * delta_time;
    transform.position.z += motion.linear_velocity.z * delta_time;

    // Вращательное движение.
    transform.rotation.x += motion.angular_velocity.x * delta_time;
    transform.rotation.y += motion.angular_velocity.y * delta_time;
    transform.rotation.z += motion.angular_velocity.z * delta_time;
}

void SceneUpdateSystem::UpdateSingleThreaded(Scene& scene, float delta_time) {
    const auto& objects = scene.GetObjects();

    // Последовательно обходим все объекты сцены.
    //
    // Здесь никакие дополнительные потоки не используются.
    // Эта реализация является baseline для измерений.
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

    // Получаем количество реально созданных worker-потоков.
    // Например
    // hardware_concurrency() == 8
    // тогда ThreadPool обычно будет содержать 8 workers.
    const std::size_t worker_count = thread_pool_.GetWorkerCount();

    /**
     * Количество задач не должно превышать количество объектов.
     *
     * Например:
     *
     * workers = 8
     * objects = 3
     *
     * Нет смысла создавать 8 задач, потому что пяти workers
     * всё равно нечего будет обрабатывать.
     *
     * Поэтому:
     *
     * job_count = min(8, 3) = 3
     */
    const std::size_t job_count = std::min(worker_count, object_count);

    /**
     * Вычисляем количество объектов в одном chunk.
     *
     * Используется целочисленное деление с округлением вверх:
     *
     *     (a + b - 1) / b
     *
     * Например:
     *
     * object_count = 1000
     * job_count = 4
     *
     * chunk_size =
     * (1000 + 4 - 1) / 4
     * = 1003 / 4
     * = 250
     *
     *
     * Если объектов 1001:
     *
     * chunk_size =
     * (1001 + 4 - 1) / 4
     * = 1004 / 4
     * = 251
     *
     * Благодаря этому последний объект не потеряется.
     */
    const std::size_t chunk_size = (object_count + job_count - 1) / job_count;

    /**
     * Каждая итерация этого цикла создаёт одну задачу ThreadPool.
     *
     * Например:
     *
     * object_count = 1000
     * job_count = 4
     * chunk_size = 250
     *
     * Получим:
     *
     * Job 0 -> [0,   250)
     * Job 1 -> [250, 500)
     * Job 2 -> [500, 750)
     * Job 3 -> [750, 1000)
     *
     * Запись [begin, end) означает:
     *
     * begin включается,
     * end не включается.
     */
    for (std::size_t job = 0; job < job_count; ++job) {
        const std::size_t begin = job * chunk_size;

        /**
         * Последний chunk может математически выйти
         * за пределы массива.
         *
         * Например:
         *
         * object_count = 1001
         * chunk_size = 251
         *
         * последний диапазон без ограничения был бы:
         *
         * [753, 1004)
         *
         * хотя objects.size() == 1001.
         *
         * Поэтому ограничиваем end через std::min().
         */
        const std::size_t end = std::min(begin + chunk_size, object_count);

        // Дополнительная защита на случай, если из-за разбиения
        // очередной диапазон начинается уже после последнего объекта.
        if (begin >= object_count) {
            break;
        }

        /**
         * Передаём задачу в ThreadPool.
         *
         * Capture:
         *
         * this
         *     нужен для вызова UpdateObject().
         *
         * &objects
         *     используем одну общую коллекцию объектов,
         *     не копируя весь std::vector.
         *
         * begin
         * end
         *     копируются по значению.
         *
         *     Это принципиально важно, потому что после Enqueue()
         *     основной поток продолжает цикл и меняет begin/end
         *     для следующей задачи.
         *
         *     Каждая lambda должна сохранить собственные границы.
         *
         * delta_time
         *     копируется по значению.
         */
        thread_pool_.Enqueue([this, &objects, begin, end, delta_time] {
            /**
             * Worker обрабатывает только принадлежащий ему диапазон.
             *
             * Разные jobs получают непересекающиеся диапазоны,
             * поэтому два worker-потока не должны одновременно
             * изменять один и тот же SceneObject.
             */
            for (std::size_t index = begin; index < end; ++index) {
                const std::shared_ptr<SceneObject>& object = objects[index];

                if (!object) {
                    continue;
                }

                UpdateObject(*object, delta_time);
            }
        });
    }

    /**
     * К этому моменту все задачи только ПОСТАВЛЕНЫ в ThreadPool.
     *
     * Некоторые из них уже могут выполняться,
     * некоторые ещё могут находиться в очереди.
     *
     * Поэтому перед выходом из UpdateMultiThreaded()
     * необходимо дождаться завершения всей работы.
     */
    thread_pool_.WaitIdle();
}