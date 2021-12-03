#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

#include <atomic>
#include <thread>

#define HANDLE_ERROR(e, msg) \
	do {                     \
		errno = e;           \
		perror(msg);         \
		exit(1);             \
	} while (0)

struct SpinLock {
	std::atomic<bool> locked{false};

	void lock() {
		while (locked.exchange(true, std::memory_order_acquire)) {
			// busy wait
			/*
			std::this_thread::yield();
			//*/
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

static pthread_attr_t GetThreadAttributes() {
	sched_param param;
	pthread_attr_t attr;
	auto err = pthread_attr_init(&attr);
	if (err) HANDLE_ERROR(1, "pthread_attr_init");
	param.sched_priority = sched_get_priority_max(SCHED_RR);

	err = pthread_attr_setschedparam(&attr, &param);
	if (err) HANDLE_ERROR(err, "pthread_attr_setschedparam");

	err = pthread_attr_setschedpolicy(&attr, SCHED_RR);
	if (err) HANDLE_ERROR(err, "setschedpolicy");

	return attr;
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
