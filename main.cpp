#include "utils.h"

#include <pthread.h>

#include <atomic>
#include <thread>


struct SpinLock {
	std::atomic<bool> locked{false};

	void lock() {
		while (locked.exchange(true, std::memory_order_acquire)) {
			// busy wait
		}
	}

	void unlock() {
		locked.store(false, std::memory_order_release);
	}
};

static void* HeavyContender(void* param) {
	auto* l = (SpinLock*)param;

	l->lock();

	return nullptr;
}

static void StartWorkerThreads(size_t count, SpinLock* lock) {
	auto attr = GetThreadAttributes();
	for (int i = 0; i < count; ++i) {
		pthread_t id;
		auto err = pthread_create(&id, &attr, HeavyContender, lock);
		if (err) HANDLE_ERROR(err, "pthread_create");
	}
}

static SpinLock g_lock;

int main() {
	g_lock.lock();
	const auto concurrency = std::thread::hardware_concurrency();
	StartWorkerThreads(concurrency, &g_lock);

	return 0;
}
