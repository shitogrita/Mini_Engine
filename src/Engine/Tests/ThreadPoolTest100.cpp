#include "ThreadPoolTest100.h"


#include "Engine/Jobs/ThreadPool.h"

#include <atomic>

bool ThreadPoolTest::Run() {
	ThreadPool pool(4);

	std::atomic<int> counter = 0;

	for (int i = 0; i < 100; ++i) {
		pool.Enqueue([&counter] {
			counter.fetch_add(1);
		});
	}

	pool.WaitIdle();

	return counter.load() == 100;
}