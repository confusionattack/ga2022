# Homework 3: Profiling with Chrome trace

Due: October 20th 2022 at 11:59pm

You will be adding a CPU profiling system to our engine with the following interface:

```
// Creates a CPU performance tracing system.
// Event capacity is the maximum number of durations that can be traced.
trace_t* trace_create(heap_t* heap, int event_capacity);

// Destroys a CPU performance tracing system.
void trace_destroy(trace_t* trace);

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
```

You can use the tracing system to time chunks of code:

```
trace_duration_push(trace, "some_work");
do_some_work();
trace_duration_pop(trace)
```

Timing information will only be captured and saved to file when enabled:

```
trace_capture_start(trace, "my_profile.json");
// ... some frames pass ...
trace_capture_end(trace);
```

We will use the Chrome trace file format to store profile data:
https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview

A sample Chrome trace file might look like this:

```
{
	"displayTimeUnit": "ns", "traceEvents": [
		{"name":"duration a","ph":"B","pid":0,"tid":"1","ts":"10"},
		{"name":"duration b","ph":"B","pid":0,"tid":"1","ts":"12"},
		{"name":"duration d","ph":"B","pid":0,"tid":"2","ts":"14"},
		{"name":"duration c","ph":"B","pid":0,"tid":"1","ts":"15"},
		{"name":"duration c","ph":"E","pid":0,"tid":"1","ts":"16"},
		{"name":"duration b","ph":"E","pid":0,"tid":"1","ts":"17"},
		{"name":"duration a","ph":"E","pid":0,"tid":"1","ts":"20"},
		{"name":"duration d","ph":"E","pid":0,"tid":"2","ts":"24"}
	]
}
```

Files like this can be opened and visualized in the Chrome web browser using `chrome://tracing`.

## Grading Criteria

Maximum total points is 15.

See homework3_test() in main.c for test case.
