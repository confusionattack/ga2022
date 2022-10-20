#include "trace.h"

#include "fs.h"
#include "heap.h"
#include "mutex.h"
#include "timer.h"
#include "trace_hash_table.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct trace_t
{
	heap_t* heap;
	trace_hash_table_t* event_stacks;
	int event_capacity;
	char* path;
	fs_t* file_system;
	mutex_t* recording_lock;
	bool is_recording;
} trace_t;

typedef struct trace_duration_t
{
	heap_t* heap;
	char* name;
	uint64_t begining_time_slice;
	uint64_t ending_time_slice;
	int thread_id;
} trace_duration_t;


trace_t* trace_create(heap_t* heap, int event_capacity)
{
	trace_t* trace = heap_alloc(heap, sizeof(trace_t), 8);
	trace->heap = heap;
	trace->event_stacks = trace_hash_table_create(heap, event_capacity);
	trace->event_capacity = event_capacity;
	trace->is_recording = false;
	trace->file_system = fs_create(trace->heap, event_capacity);
	trace->recording_lock = mutex_create();
	return trace;
}

void trace_destroy(trace_t* trace)
{
	mutex_destroy(trace->recording_lock);
	trace_hash_table_destroy(trace->event_stacks);
	fs_destroy(trace->file_system);
	heap_free(trace->heap, trace);
}

trace_duration_t* trace_duration_create(trace_t* trace, const char* name)
{
	trace_duration_t* duration = heap_alloc(trace->heap, sizeof(trace_duration_t), 8);
	duration->begining_time_slice = timer_ticks_to_us(timer_get_ticks());
	duration->ending_time_slice = 0;
	duration->heap = trace->heap;
	duration->name = (char*) name;
	duration->thread_id = GetCurrentThreadId();
	return duration;
}

void trace_duration_destroy(trace_duration_t* duration)
{
	heap_free(duration->heap, duration);
}

void trace_duration_set_end_time(trace_duration_t* duration)
{
	duration->ending_time_slice = timer_ticks_to_us(timer_get_ticks());
}

void trace_duration_push(trace_t* trace, const char* name)
{
	mutex_lock(trace->recording_lock);
	if (!trace->is_recording)
	{
		mutex_unlock(trace->recording_lock);
		return;
	}

	trace_duration_t* duration = trace_duration_create(trace, name);

	trace_hash_table_push(trace->event_stacks, (int)GetCurrentThreadId(), duration);
	mutex_unlock(trace->recording_lock);
}

void trace_duration_pop(trace_t* trace)
{
	mutex_lock(trace->recording_lock);
	if (!trace->is_recording)
	{
		mutex_unlock(trace->recording_lock);
		return;
	}

	trace_hash_table_pop_off_stack(trace->event_stacks, (int)GetCurrentThreadId());
	mutex_unlock(trace->recording_lock);
}

void trace_capture_start(trace_t* trace, const char* path)
{
	mutex_lock(trace->recording_lock);

	if (trace->is_recording)
	{
		return;
	}

	trace->path = (char*)path;

	trace_hash_table_destroy(trace->event_stacks);
	trace->event_stacks = trace_hash_table_create(trace->heap, trace->event_capacity);

	trace->is_recording = true;
	mutex_unlock(trace->recording_lock);
}

void trace_capture_stop(trace_t* trace)
{
	mutex_lock(trace->recording_lock);
	if (!trace->is_recording)
	{
		mutex_unlock(trace->recording_lock);
		return;
	}

	char* json_heading = "{\n\t\"displayTimeUnit\": \"ns\", \"traceEvents\" : [";
	char* name_text = "\n\t\t{\"name\":\"";
	char* phase_text = "\",\"ph\":\"";
	char* pid_tid_text = "\", \"pid\":0, \"tid\":\"";
	char* time_scale_text = "\", \"ts\":\"";
	char* closing_text = "\"}";
	char* json_ending = "\n\t]\n}\n";

	fs_write_clear(trace->file_system, (char*)trace->path, json_heading, strlen(json_heading), true, true);

	bool is_first_element = true;
	
	int num_events = trace_hash_table_size(trace->event_stacks);
	for(int index = 0; index < num_events; index++)
	{
		trace_duration_t* event = trace_hash_table_get_event(trace->event_stacks, index);

		for (int i = 0; i < 2; i++)
		{
			size_t buffer_size = strlen(name_text) + strlen(event->name) + strlen(phase_text) + 1 
				+ strlen(pid_tid_text) + 10 + strlen(time_scale_text) + 20 + strlen(closing_text);

			if (is_first_element)
			{
				buffer_size++;
			}

			char* buffer = heap_alloc(event->heap, buffer_size, 8);

			char phase;
			uint64_t time_slice;
			if (i == 0)
			{
				phase = 'B';
				time_slice = event->begining_time_slice;
			}
			else
			{
				phase = 'E';
				time_slice = event->ending_time_slice;
			}			

			size_t num_bites_written = 0;
			if (is_first_element)
			{
				num_bites_written = snprintf(buffer, buffer_size, "%s%s%s%c%s%d%s%llu%s",
					name_text, event->name, phase_text, phase, pid_tid_text, event->thread_id, time_scale_text, time_slice, closing_text);
				is_first_element = false;
			}
			else
			{
				num_bites_written = snprintf(buffer, buffer_size, ",%s%s%s%c%s%d%s%llu%s",
					name_text, event->name, phase_text, phase, pid_tid_text, event->thread_id, time_scale_text, time_slice, closing_text);
			}

			fs_work_t* line_worker = fs_write_clear(trace->file_system, (char*)trace->path, (char*) buffer, num_bites_written, false, false);

			fs_work_wait(line_worker);
			fs_work_destroy(line_worker);
			heap_free(event->heap, buffer);

			if (event->ending_time_slice < event->begining_time_slice)
			{
				break;
			}
		}
	}

	fs_write_clear(trace->file_system, (char*)trace->path, json_ending, strlen(json_ending), false, true);

	trace->is_recording = false;
	mutex_unlock(trace->recording_lock);
}
