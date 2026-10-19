#include "Engine/Scene/SceneUpdateSystem.h"

#include <algorithm>
#include <memory>

/**
 * @brief Создаёт систему обновления сцены.
 *
 * ThreadPool создаётся один раз
 * вместе с SceneUpdateSystem
 * и затем переиспользуется между кадрами.
 *
 * Если количество worker-потоков явно не указано,
 * используется значение по умолчанию ThreadPool.
 */
SceneUpdateSystem::SceneUpdateSystem() {
    thread_pool_ = std::make_unique<ThreadPool>();
}

/**
 * @brief Обновляет состояние Scene
 * в выбранном режиме выполнения.
 *
 * Метод является общей точкой входа
 * в систему обновления сцены.
 *
 * В зависимости от ExecutionMode вызывается:
 *
 * - UpdateSingleThreaded() — последовательное обновление;
 * - UpdateMultiThreaded() — параллельное обновление через ThreadPool.
 *
 * В обоих режимах логика изменения отдельного SceneObject
 * остаётся одинаковой и находится в UpdateObject().
 *
 * @param scene Сцена, объекты которой необходимо обновить.
 * @param delta_time Время между обновлениями.
 * @param mode Режим выполнения Scene Update.
 */
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

/**
 * @brief Изменяет количество worker-потоков ThreadPool.
 *
 * Нулевое значение автоматически заменяется на 1,
 * поскольку ThreadPool должен содержать
 * хотя бы один worker.
 *
 * Если текущее количество workers уже совпадает
 * с запрошенным, ThreadPool не пересоздаётся.
 *
 * При изменении количества потоков система:
 *
 * 1. ожидает завершения уже поставленных задач;
 * 2. уничтожает существующий ThreadPool;
 * 3. создаёт новый ThreadPool
 *    с указанным количеством workers.
 *
 * @param worker_count Требуемое количество worker-потоков.
 */
void SceneUpdateSystem::SetWorkerCount(std::size_t worker_count) {
    /**
     * Не допускаем создание ThreadPool
     * с нулевым количеством worker-потоков.
     */
    if (worker_count == 0) {
        worker_count = 1;
    }

    /**
     * Если конфигурация уже соответствует требуемой,
     * пересоздание ThreadPool не требуется.
     */
    if (thread_pool_ && thread_pool_->GetWorkerCount() == worker_count) {
        return;
    }

    /**
     * Перед заменой ThreadPool дожидаемся,
     * пока все уже поставленные задачи завершатся.
     *
     * Это предотвращает ситуацию,
     * при которой старые workers продолжают
     * работать с объектами Scene
     * во время уничтожения пула.
     */
    if (thread_pool_) {
        thread_pool_->WaitIdle();
    }

    /**
     * std::unique_ptr автоматически уничтожает
     * предыдущий ThreadPool.
     *
     * После этого создаётся новый пул
     * с необходимым количеством workers.
     */
    thread_pool_ = std::make_unique<ThreadPool>(worker_count);
}

/**
 * @brief Возвращает текущее количество worker-потоков.
 *
 * Если ThreadPool отсутствует,
 * возвращается 0.
 *
 * @return Количество workers текущего ThreadPool.
 */
std::size_t SceneUpdateSystem::GetWorkerCount() const {
    if (!thread_pool_) {
        return 0;
    }

    return thread_pool_->GetWorkerCount();
}

/**
 * @brief Обновляет состояние одного SceneObject.
 *
 * Метод содержит общую логику движения,
 * используемую как SingleThreaded,
 * так и MultiThreaded режимом.
 *
 * Если MotionState объекта выключен,
 * Transform не изменяется.
 *
 * При включённом MotionState:
 *
 * position += linear_velocity * delta_time
 *
 * rotation += angular_velocity * delta_time
 *
 * Обновление выполняется независимо
 * для компонентов X, Y и Z.
 *
 * @param object SceneObject, который необходимо обновить.
 * @param delta_time Время между обновлениями.
 */
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

/**
 * @brief Последовательно обновляет все SceneObject.
 *
 * SingleThreaded используется как базовый режим,
 * в котором объекты сцены обрабатываются
 * один за другим в текущем потоке.
 *
 * nullptr-элементы Scene пропускаются.
 *
 * Для каждого существующего объекта
 * вызывается общая функция UpdateObject().
 *
 * @param scene Обновляемая сцена.
 * @param delta_time Время между обновлениями.
 */
void SceneUpdateSystem::UpdateSingleThreaded(Scene& scene, float delta_time) {
    const auto& objects = scene.GetObjects();

    for (const std::shared_ptr<SceneObject>& object : objects) {
        if (!object) {
            continue;
        }

        UpdateObject(*object, delta_time);
    }
}

/**
 * @brief Параллельно обновляет SceneObject через ThreadPool.
 *
 * Объекты Scene разделяются на несколько
 * непрерывных диапазонов — chunks.
 *
 * Каждый chunk передаётся ThreadPool
 * как отдельная задача.
 *
 * Количество jobs ограничивается одновременно:
 *
 * - количеством доступных worker-потоков;
 * - количеством объектов Scene.
 *
 * После постановки всех jobs вызывается WaitIdle(),
 * поэтому метод завершает работу только после того,
 * как все изменения Transform будут выполнены.
 *
 * Во время выполнения поставленных задач
 * контейнер Scene::GetObjects()
 * не должен изменять свою структуру.
 *
 * @param scene Обновляемая сцена.
 * @param delta_time Время между обновлениями.
 */
void SceneUpdateSystem::UpdateMultiThreaded(Scene& scene, float delta_time) {
    const auto& objects = scene.GetObjects();

    const std::size_t object_count = objects.size();

    if (object_count == 0) {
        return;
    }

    /**
     * Обычно ThreadPool уже существует,
     * поскольку создаётся в конструкторе.
     *
     * Проверка оставлена для гарантии наличия пула
     * непосредственно перед многопоточным обновлением.
     */
    if (!thread_pool_) {
        thread_pool_ = std::make_unique<ThreadPool>();
    }

    const std::size_t worker_count = thread_pool_->GetWorkerCount();

    /**
     * Не создаём больше jobs,
     * чем имеется workers или SceneObject.
     *
     * Например:
     *
     * workers = 10
     * objects = 4
     *
     * job_count = 4.
     */
    const std::size_t job_count =
        std::min(worker_count, object_count);

    /**
     * Вычисляем размер одного непрерывного chunk.
     *
     * Формула выполняет целочисленное округление вверх:
     *
     * chunk_size =
     * ceil(object_count / job_count).
     *
     * Например:
     *
     * object_count = 25 000
     * job_count = 10
     *
     * chunk_size = 2 500.
     */
    const std::size_t chunk_size =
        (object_count + job_count - 1) / job_count;

    /**
     * Создаём до job_count независимых задач.
     *
     * Каждая задача получает собственный диапазон
     * индексов [begin, end).
     */
    for (std::size_t job = 0; job < job_count; ++job) {
        const std::size_t begin =
            job * chunk_size;

        const std::size_t end =
            std::min(
                begin + chunk_size,
                object_count
            );

        /**
         * При неравномерном распределении объектов
         * один из последних вычисленных диапазонов
         * может начинаться уже за пределами контейнера.
         */
        if (begin >= object_count) {
            break;
        }

        /**
         * begin, end и delta_time
         * захватываются lambda по значению.
         *
         * objects захватывается по ссылке.
         *
         * Контейнер остаётся доступен
         * до завершения WaitIdle(),
         * поэтому workers могут обращаться
         * к своим диапазонам объектов.
         *
         * Каждый job работает
         * с отдельным диапазоном индексов.
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
     * Ожидаем завершения всех jobs текущего обновления.
     *
     * После возврата из WaitIdle()
     * workers больше не изменяют Transform
     * объектов этого Scene Update.
     *
     * Таким образом, дальнейшая обработка кадра
     * начинается уже после завершения
     * многопоточного CPU update.
     */
    thread_pool_->WaitIdle();
}