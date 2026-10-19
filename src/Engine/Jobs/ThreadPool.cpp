#include "Engine/Jobs/ThreadPool.h"

#include <utility>

/**
 * @brief Создаёт пул постоянных worker-потоков.
 *
 * ThreadPool используется для выполнения CPU-задач движка
 * без создания нового std::thread для каждой отдельной операции.
 *
 * Количество worker-потоков задаётся при создании объекта.
 * Если передано значение 0, создаётся как минимум один worker.
 *
 * Каждый worker сразу запускает WorkerLoop() и ожидает
 * появления задач во внутренней очереди tasks_.
 *
 * @param worker_count Количество worker-потоков в пуле.
 */
ThreadPool::ThreadPool(std::size_t worker_count) {
    /**
     * Пул не должен существовать без worker-потоков.
     *
     * Значение 0 заменяется на 1,
     * чтобы ThreadPool сохранял работоспособность.
     */
    if (worker_count == 0) {
        worker_count = 1;
    }

    /**
     * Заранее резервируем память под std::thread,
     * чтобы избежать лишних перераспределений vector
     * во время создания workers.
     */
    workers_.reserve(worker_count);

    /**
     * Создаём постоянные worker-потоки.
     *
     * Каждый поток выполняет WorkerLoop()
     * до момента остановки ThreadPool.
     */
    for (std::size_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this] {
            WorkerLoop();
        });
    }
}

/**
 * @brief Останавливает ThreadPool и ожидает завершения worker-потоков.
 *
 * Сначала под защитой mutex_ устанавливается stopping_ = true.
 *
 * После этого task_condition_ уведомляет все worker-потоки,
 * включая те, которые в данный момент ожидают новые задачи.
 *
 * WorkerLoop завершает поток только когда одновременно:
 *
 * - stopping_ == true;
 * - очередь tasks_ пуста.
 *
 * Поэтому уже поставленные в очередь задачи
 * продолжают обрабатываться перед завершением workers.
 *
 * После отправки сигнала остановки деструктор вызывает join()
 * для каждого std::thread и ожидает полного завершения пула.
 */
ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }

    task_condition_.notify_all();

    for (std::thread& worker : workers_) {
        worker.join();
    }
}

/**
 * @brief Добавляет новую задачу в очередь ThreadPool.
 *
 * Задача помещается во внутреннюю FIFO-очередь tasks_
 * под защитой mutex_.
 *
 * После добавления task_condition_ пробуждает
 * один ожидающий worker-поток.
 *
 * Если ThreadPool уже находится в состоянии остановки,
 * новая задача не добавляется.
 *
 * @param task Функция без параметров и возвращаемого значения,
 * которую необходимо выполнить в одном из worker-потоков.
 */
void ThreadPool::Enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (stopping_) {
            return;
        }

        tasks_.push(std::move(task));
    }

    /**
     * Для одной новой задачи достаточно разбудить
     * один ожидающий worker.
     */
    task_condition_.notify_one();
}

/**
 * @brief Блокирует вызывающий поток до полного завершения текущей работы.
 *
 * ThreadPool считается idle только тогда, когда одновременно:
 *
 * - очередь tasks_ пуста;
 * - ни один worker не выполняет задачу.
 *
 * Проверка только tasks_.empty() была бы недостаточной,
 * потому что worker мог уже извлечь последнюю задачу из очереди,
 * но всё ещё продолжать её выполнение.
 *
 * Поэтому дополнительно отслеживается active_workers_.
 *
 * Метод используется как точка синхронизации между
 * вызывающим потоком и worker-потоками.
 */
void ThreadPool::WaitIdle() {
    std::unique_lock<std::mutex> lock(mutex_);

    idle_condition_.wait(lock, [this] {
        return tasks_.empty() && active_workers_ == 0;
    });
}

/**
 * @brief Возвращает количество worker-потоков ThreadPool.
 *
 * Количество соответствует размеру workers_
 * и остаётся постоянным на протяжении жизни
 * конкретного экземпляра ThreadPool.
 *
 * @return Количество worker-потоков.
 */
std::size_t ThreadPool::GetWorkerCount() const {
    return workers_.size();
}

/**
 * @brief Основной цикл worker-потока.
 *
 * Каждый worker выполняет этот метод с момента своего создания
 * до остановки ThreadPool.
 *
 * Общая последовательность работы:
 *
 * 1. Worker ожидает новую задачу или сигнал остановки.
 * 2. Извлекает первую задачу из очереди.
 * 3. Увеличивает active_workers_.
 * 4. Освобождает mutex_.
 * 5. Выполняет задачу.
 * 6. Снова захватывает mutex_.
 * 7. Уменьшает active_workers_.
 * 8. Если работа полностью завершена,
 *    уведомляет поток, ожидающий в WaitIdle().
 *
 * mutex_ не удерживается во время выполнения task().
 * Благодаря этому остальные workers могут одновременно
 * получать следующие задачи из очереди.
 */
void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            /**
             * Worker спит, пока:
             *
             * - не появится новая задача;
             * - либо ThreadPool не начнёт остановку.
             */
            task_condition_.wait(lock, [this] {
                return stopping_ || !tasks_.empty();
            });

            /**
             * После начала остановки worker завершается
             * только тогда, когда очередь полностью пуста.
             *
             * Если задачи ещё присутствуют,
             * они продолжают выполняться.
             */
            if (stopping_ && tasks_.empty()) {
                return;
            }

            /**
             * Забираем первую задачу из FIFO-очереди
             * и помечаем worker как активный.
             */
            task = std::move(tasks_.front());
            tasks_.pop();

            active_workers_++;
        }

        /**
         * Сама задача выполняется без удержания mutex_.
         *
         * Это позволяет нескольким workers
         * работать параллельно.
         */
        task();

        {
            std::lock_guard<std::mutex> lock(mutex_);

            active_workers_--;

            /**
             * Если очередь пуста и ни один worker
             * больше не выполняет задачу,
             * ThreadPool перешёл в состояние idle.
             *
             * Уведомляем WaitIdle().
             */
            if (tasks_.empty() && active_workers_ == 0) {
                idle_condition_.notify_one();
            }
        }
    }
}