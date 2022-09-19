#include "atomic.h"
#include "debug.h"
#include "event.h"
#include "mutex.h"
#include "semaphore.h"
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

	// Three things wrong?
	// 1. Read and the write are racing.
	//    Read can happen, then another thread modifies, then the write happens.
	// 2. The compiler.
	// 3. Cache is wrong.

	// Thread 1 comes in.
	//    Loop 0, loads thread_data->counter into its *own* L1 cache.
	//    Loop 1, uses thread_data->counter from its *own* L1 cache.
	//    Loop 100000, uses thread_data->counter from its *own* L1 cache.

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

#define QUEUE_MAX 16

typedef struct queue_t
{
	semaphore_t* count;
	int items[QUEUE_MAX];
	int head_index;
	int tail_index;
} queue_t;

static void queue_push(queue_t* queue, int item)
{
	// IMPLEMENT ME!
}

static int queue_pop(queue_t* queue)
{
	// IMPLEMENT ME!
	return -1;
}

static int consumer_function(void* user)
{
	while (1)
	{
		int item = queue_pop(user);
		if (item == -1)
		{
			break;
		}
		debug_print(k_print_warning, "Popped item: %d\n", item);
	}
	return 0;
}

void lecture7_queue_test()
{
	queue_t queue =
	{
		.count = semaphore_create(0, QUEUE_MAX),
	};

	thread_t* consumer_thread = thread_create(consumer_function, &queue);

	queue_push(&queue, 0);
	queue_push(&queue, 1);
	queue_push(&queue, 2);
	queue_push(&queue, 3);
	queue_push(&queue, -1);

	thread_destroy(consumer_thread);
}


// Thread 1:
// assert(*write_last, 99);
// *write_here = 5;
// *write_there = 6;
// atomic_store(*write_last, 7);

// Thread 2:
// int value = atomic_load(*write_last);
// IF value == 7:
//    assert(*write_here == 5);
//    assert(*write_there == 6);
//


struct foo
{
	int x;
	char padding[60];
	int y;
};

//Thread 1:
//foo->x = 12;

//Thread 2:
//foo->y = 45;

//Main thread waited for threads 1 and 2 to be done:
// What is the value of x and y?
// 1. x=12, y=45
// 2. x=12, y=?
// 3. x=?, y=45
