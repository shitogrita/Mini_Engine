#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * @brief Пул постоянных worker-потоков для выполнения CPU-задач Mini Engine.
 *
 * ThreadPool создаёт фиксированное количество std::thread
 * и использует их повторно для выполнения задач,
 * помещаемых во внутреннюю очередь.
 *
 * Такой подход позволяет избежать постоянного создания
 * и уничтожения потоков при каждом обновлении сцены.
 *
 * Основные обязанности ThreadPool:
 *
 * - хранение worker-потоков;
 * - хранение очереди задач;
 * - синхронизация доступа к очереди;
 * - пробуждение workers при появлении новой работы;
 * - ожидание полного завершения текущих задач;
 * - корректная остановка всех потоков при уничтожении.
 *
 * ThreadPool используется только для CPU-логики.
 * OpenGL-вызовы должны выполняться в потоке,
 * которому принадлежит OpenGL-контекст.
 */
class ThreadPool {
public:
    /**
     * @brief Создаёт пул worker-потоков.
     *
     * По умолчанию количество workers определяется
     * через std::thread::hardware_concurrency().
     *
     * Если передано значение 0,
     * реализация конструктора создаёт один worker.
     *
     * @param worker_count Количество worker-потоков.
     */
    explicit ThreadPool(std::size_t worker_count = std::thread::hardware_concurrency());

    /**
     * @brief Останавливает пул и ожидает завершения всех workers.
     *
     * Деструктор переводит ThreadPool в состояние остановки,
     * пробуждает ожидающие потоки и выполняет join()
     * для каждого std::thread.
     *
     * Уже помещённые в очередь задачи обрабатываются
     * перед окончательным завершением worker-потоков.
     */
    ~ThreadPool();

    /**
     * @brief Копирование ThreadPool запрещено.
     *
     * Объект владеет потоками, mutex,
     * condition_variable и очередью задач,
     * поэтому копирование не допускается.
     */
    ThreadPool(const ThreadPool&) = delete;

    /**
     * @brief Копирующее присваивание ThreadPool запрещено.
     */
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Добавляет новую задачу в очередь выполнения.
     *
     * Задача сохраняется во внутренней FIFO-очереди tasks_.
     *
     * После добавления пробуждается один worker-поток.
     *
     * Если ThreadPool уже находится
     * в состоянии остановки, задача не добавляется.
     *
     * @param task Функция без параметров и возвращаемого значения,
     * которую необходимо выполнить в одном из workers.
     */
    void Enqueue(std::function<void()> task);

    /**
     * @brief Ожидает полного завершения текущей работы ThreadPool.
     *
     * Метод блокирует вызывающий поток до тех пор,
     * пока одновременно:
     *
     * - очередь tasks_ не станет пустой;
     * - active_workers_ не станет равным 0.
     *
     * Это гарантирует, что задачи не только извлечены
     * из очереди, но и полностью завершены.
     */
    void WaitIdle();

    /**
     * @brief Возвращает количество worker-потоков.
     *
     * @return Текущее количество std::thread в workers_.
     */
    std::size_t GetWorkerCount() const;

private:
    /**
     * @brief Основной цикл одного worker-потока.
     *
     * Worker ожидает появления задачи,
     * извлекает её из очереди,
     * выполняет без удержания mutex_
     * и обновляет счётчик active_workers_.
     *
     * Цикл завершается только тогда,
     * когда ThreadPool остановлен
     * и очередь задач полностью пуста.
     */
    void WorkerLoop();

    /**
     * @brief Постоянные worker-потоки ThreadPool.
     */
    std::vector<std::thread> workers_;

    /**
     * @brief FIFO-очередь задач.
     *
     * Доступ к контейнеру синхронизируется через mutex_.
     */
    std::queue<std::function<void()>> tasks_;

    /**
     * @brief Mutex, защищающий внутреннее состояние ThreadPool.
     *
     * Используется при работе с:
     *
     * - tasks_;
     * - stopping_;
     * - active_workers_.
     */
    mutable std::mutex mutex_;

    /**
     * @brief Condition variable для ожидания новых задач.
     *
     * Worker-потоки блокируются на task_condition_,
     * пока очередь пуста и ThreadPool не остановлен.
     */
    std::condition_variable task_condition_;

    /**
     * @brief Condition variable для ожидания состояния idle.
     *
     * Используется методом WaitIdle().
     *
     * Уведомление отправляется,
     * когда очередь задач пуста
     * и active_workers_ равен 0.
     */
    std::condition_variable idle_condition_;

    /**
     * @brief Флаг остановки ThreadPool.
     *
     * После установки true новые задачи
     * больше не принимаются.
     */
    bool stopping_ = false;

    /**
     * @brief Количество workers,
     * которые в данный момент выполняют задачи.
     *
     * Поле необходимо для корректной реализации WaitIdle(),
     * потому что пустая очередь сама по себе
     * не означает завершение всех задач.
     */
    std::size_t active_workers_ = 0;
};