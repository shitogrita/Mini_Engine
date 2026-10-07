#include "Engine/Jobs/ThreadPool.h"

#include <utility>

ThreadPool::ThreadPool(std::size_t worker_count) {
	if (worker_count == 0) {
		worker_count = 1;
	}

	workers_.reserve(worker_count);

	for (std::size_t i = 0; i < worker_count; ++i) {
		workers_.emplace_back([this] {
			WorkerLoop();
		});
	}
}

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


void ThreadPool::Enqueue(std::function<void()> task) {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (stopping_) {
			return;
		}
		tasks_.push(std::move(task));
	}
	task_condition_.notify_one();
}


void ThreadPool::WaitIdle() {
	std::unique_lock<std::mutex> lock(mutex_);
	idle_condition_.wait(lock, [this] {
		return tasks_.empty() && active_workers_ == 0;
	});
}

std::size_t ThreadPool::GetWorkerCount() const {
	return workers_.size();
}

void ThreadPool::WorkerLoop() {
	while (true) {
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(mutex_);
			task_condition_.wait(lock,[this] {
				return stopping_ || !tasks_.empty();
			});
			if (stopping_ && tasks_.empty()) {
				return;
			}
			task = std::move(tasks_.front());
			tasks_.pop();
			active_workers_++;
		}
		task();
		{
			std::lock_guard<std::mutex> lock(mutex_);
			active_workers_--;
			if (tasks_.empty() && active_workers_ == 0) {
				idle_condition_.notify_one();
			}
		}
	}
}