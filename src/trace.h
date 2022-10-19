#pragma once

typedef struct heap_t heap_t;

typedef struct trace_t trace_t;
typedef struct trace_duration_t trace_duration_t;

// Creates a CPU performance tracing system.
// Event capacity is the maximum number of durations that can be traced.
trace_t* trace_create(heap_t* heap, int event_capacity);

// Destroys a CPU performance tracing system.
void trace_destroy(trace_t* trace);

// Creates a duration event for a CPU performance tracing system.
// Name is the name of the duration event. 
// Event phase is the "phase" of the event. 'B' is the begining of the duraiton
// and 'E' is the end of the duration.
trace_duration_t* trace_duration_create(heap_t* heap, char* name, char event_phase);

// Destroys a duration event.
void trace_duration_destroy(trace_duration_t* duration);

// Begin tracing a named duration on the current thread.
// It is okay to nest multiple durations at once.
void trace_duration_push(trace_t* trace, const char* name);

// End tracing the currently active duration on the current thread.
void trace_duration_pop(trace_t* trace);

// Start recording trace events.
// A Chrome trace file will be written to path.
void trace_capture_start(trace_t* trace, const char* path);

// Stop recording trace events.
void trace_capture_stop(trace_t* trace);




// Tests ======================================================
char* get_duration_name(trace_duration_t* duration);
