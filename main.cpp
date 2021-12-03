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

	for (;;) {
		l->lock();
		// prevent optimization
		asm volatile("nop\n\t" : "=m"(l));
		l->unlock();
	}

	return nullptr;
}

static SpinLock g_lock;

int main() {
	g_lock.lock();

	struct sched_param param;
	pthread_attr_t attr;
	auto err = pthread_attr_init(&attr);
	if (err) HANDLE_ERROR(1, "pthread_attr_init");
	param.sched_priority = sched_get_priority_max(SCHED_RR);

	err = pthread_attr_setschedparam(&attr, &param);
	if (err) HANDLE_ERROR(err, "pthread_attr_setschedparam");

	err = pthread_attr_setschedpolicy(&attr, SCHED_RR);
	if (err) HANDLE_ERROR(err, "setschedpolicy");

	// end up with virtual cores + 1 threads in total
	for (int i = 0; i < 16; ++i) {
		pthread_t id;
		err = pthread_create(&id, &attr, HeavyContender, &g_lock);
		if (err) HANDLE_ERROR(err, "pthread_create");
	}

	sleep(1);
	g_lock.unlock();

	return 0;
}
