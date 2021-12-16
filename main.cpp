#include "utils.h"

#include <pthread.h>

#include <atomic>
#include <thread>

struct SpinLock {
	std::atomic<bool> locked{false};

	void lock() {
		while (locked.exchange(true, std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}

	void unlock() {
		locked.store(false, std::memory_order_release);
	}
};

static SpinLock g_lock;

static void* HeavyContender(void* /*param*/) {
	g_lock.lock();	
	g_lock.unlock();

	return nullptr;
}

static void StartWorkerThreads(size_t count) {
	auto attr = GetThreadAttributes();
	for (int i = 0; i < count; ++i) {
		pthread_t id;
		auto err = pthread_create(&id, &attr, HeavyContender, nullptr);
		if (err) HANDLE_ERROR(err, "pthread_create");
	}
}

int main() {
	g_lock.lock();
	const auto concurrency = std::thread::hardware_concurrency();
	StartWorkerThreads(concurrency);
	g_lock.unlock();

	return 0;
}
