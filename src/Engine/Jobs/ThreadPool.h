#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
	explicit ThreadPool(std::size_t worker_count = std::thread::hardware_concurrency());
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	void Enqueue(std::function<void()> task);
	void WaitIdle();

	std::size_t GetWorkerCount() const;

private:
	void WorkerLoop();

	std::vector<std::thread> workers_;
	std::queue<std::function<void()>> tasks_;

	mutable std::mutex mutex_;

	std::condition_variable task_condition_; // есть работа
	std::condition_variable idle_condition_; // Закончились ли задачи на потоках

	bool stopping_ = false;

	std::size_t active_workers_ = 0;
};