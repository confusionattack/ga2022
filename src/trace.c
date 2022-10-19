#include "trace.h"

#include "heap.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stddef.h>
#include <stdint.h>

typedef struct trace_t
{
	heap_t* heap;
} trace_t;

typedef struct trace_duration_t
{
	heap_t* heap;
	char* name;
	uint64_t time_slice;
	int thread_id;
	int process_id;
	char event_phase;
} trace_duration_t;


trace_t* trace_create(heap_t* heap, int event_capacity)
{
	return NULL;
}

void trace_destroy(trace_t* trace)
{

}

trace_duration_t* trace_duration_create(heap_t* heap, char* name, char event_phase)
{
	trace_duration_t* duration = heap_alloc(heap, sizeof(trace_duration_t), 8);
	duration->heap = heap;
	duration->name = name;
	duration->thread_id = GetCurrentThreadId();
	duration->process_id = GetCurrentProcessId();
	duration->event_phase = event_phase;
	duration->time_slice = 0;
	return duration;
}

void trace_duration_destroy(trace_duration_t* duration)
{
	heap_free(duration->heap, duration);
}

void trace_duration_push(trace_t* trace, const char* name)
{

}

void trace_duration_pop(trace_t* trace)
{

}

void trace_capture_start(trace_t* trace, const char* path)
{

}

void trace_capture_stop(trace_t* trace)
{

}







// TESTING =================================================
char* get_duration_name(trace_duration_t* duration)
{
	return duration->name;
}
