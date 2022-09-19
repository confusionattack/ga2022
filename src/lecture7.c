#include "atomic.h"
#include "debug.h"
#include "event.h"
#include "mutex.h"
#include "thread.h"

#include <windows.h>

typedef struct thread_data_t
{
	int* counter;
	mutex_t* mutex;
	event_t* start;
} thread_data_t;

static int no_synchronization_func(void* user)
{
	thread_data_t* thread_data = user;
	event_wait(thread_data->start);

	DWORD t0 = timeGetTime();

	for (int i = 0; i < 100000; ++i)
	{
		*thread_data->counter = *thread_data->counter + 1;
	}

	return timeGetTime() - t0;
}

static int atomic_load_store_func(void* user)
{
	thread_data_t* thread_data = user;
	event_wait(thread_data->start);

	DWORD t0 = timeGetTime();

	for (int i = 0; i < 100000; ++i)
	{
		atomic_store(thread_data->counter, atomic_load(thread_data->counter) + 1);
	}

	return timeGetTime() - t0;
}

static int atomic_increment_func(void* user)
{
	thread_data_t* thread_data = user;
	event_wait(thread_data->start);

	DWORD t0 = timeGetTime();

	for (int i = 0; i < 100000; ++i)
	{
		atomic_increment(thread_data->counter);
	}

	return timeGetTime() - t0;
}

static int mutex_func(void* user)
{
	thread_data_t* thread_data = user;
	event_wait(thread_data->start);

	DWORD t0 = timeGetTime();

	for (int i = 0; i < 100000; ++i)
	{
		mutex_lock(thread_data->mutex);
		*thread_data->counter = *thread_data->counter + 1;
		mutex_unlock(thread_data->mutex);
	}

	return timeGetTime() - t0;
}

static void run_timed_test(int (*thread_func)(void*), const char* name)
{
	int counter = 0;
	thread_data_t thread_data =
	{
		.counter = &counter,
		.mutex = mutex_create(),
		.start = event_create(),
	};

	// Create threads.
	thread_t* threads[8];
	for (int i = 0; i < _countof(threads); ++i)
	{
		threads[i] = thread_create(thread_func, &thread_data);
	}

	// Go!
	event_signal(thread_data.start);

	// Wait for threads to be done.
	int duration = 0;
	for (int i = 0; i < _countof(threads); ++i)
	{
		duration += thread_destroy(threads[i]);
	}
	mutex_destroy(thread_data.mutex);
	event_destroy(thread_data.start);

	debug_print(k_print_warning, "%s duration=%dms, counter=%d\n", name, duration, counter);
}

void lecture7_thread_test()
{
	run_timed_test(no_synchronization_func, "no_synchronization");
	run_timed_test(atomic_load_store_func, "atomic_load_store");
	run_timed_test(atomic_increment_func, "atomic_increment");
	run_timed_test(mutex_func, "mutex");
}
