#include "atomic.h"
#include "debug.h"
#include "fs.h"
#include "heap.h"
#include "mutex.h"
#include "thread.h"
#include "timer.h"
#include "timer_object.h"
#include "trace.h"
#include "wm.h"

#include <assert.h>
#include <stdio.h>

#include <windows.h>

static void homework3_test();

int main(int argc, const char* argv[])
{
	debug_install_exception_handler();
	timer_startup();

	homework3_test();

	debug_set_print_mask(k_print_info | k_print_warning | k_print_error);

	heap_t* heap = heap_create(2 * 1024 * 1024);
	wm_window_t* window = wm_create(heap);
	timer_object_t* root_time = timer_object_create(heap, NULL);

	// THIS IS THE MAIN LOOP!
	while (!wm_pump(window))
	{
		timer_object_update(root_time);

		int x, y;
		wm_get_mouse_move(window, &x, &y);

		uint32_t mask = wm_get_mouse_mask(window);

		uint32_t now = timer_ticks_to_ms(timer_get_ticks());
		debug_print(
			k_print_info,
			"T=%dms, MOUSE mask=%x move=%dx%d\n",
			timer_object_get_ms(root_time),
			mask,
			x, y);
	}

	timer_object_destroy(root_time);
	wm_destroy(window);
	heap_destroy(heap);

	return 0;
}

static void homework3_slower_function(trace_t* trace)
{
	trace_duration_push(trace, "homework3_slower_function");
	thread_sleep(200);
	trace_duration_pop(trace);
}

static void homework3_slow_function(trace_t* trace)
{
	trace_duration_push(trace, "homework3_slow_function");
	thread_sleep(100);
	homework3_slower_function(trace);
	trace_duration_pop(trace);
}

static int homework3_test_func(void* data)
{
	trace_t* trace = data;
	homework3_slow_function(trace);
	return 0;
}

static void homework3_test()
{
	heap_t* heap = heap_create(4096);

	// Create the tracing system with at least space for 100 *captured* events.
	// Each call to trace_duration_push is an event.
	// Each call to trace_duration_pop is an event.
	// Before trace_capture_start is called, and after trace_capture_stop is called,
	// duration events should not be generated.
	trace_t* trace = trace_create(heap, 100);

	// Capturing has *not* started so these calls can safely be ignored.
	trace_duration_push(trace, "should be ignored");
	trace_duration_pop(trace);

	// Start capturing events.
	// Eventually we will want to write events to a file -- "trace.json".
	// However we should *not* write to the file for each call to trace_duration_push or
	// trace_duration_pop. That would be much too slow. Instead, events should be buffered
	// (up to event_capacity) before writing to a file. For purposes of this homework,
	// it is entirely fine if you only capture the first event_capacity count events and
	// ignore any additional events.
	trace_capture_start(trace, "trace.json");

	// Create a thread that will push/pop duration events.
	thread_t* thread = thread_create(homework3_test_func, trace);

	// Call a function that will push/pop duration events.
	homework3_slow_function(trace);

	// Wait for thread to finish.
	thread_destroy(thread);

	// Finish capturing. Write the trace.json file in Chrome tracing format.
	trace_capture_stop(trace);

	trace_destroy(trace);

	heap_destroy(heap);
}
