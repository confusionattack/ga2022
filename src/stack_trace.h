#include <stdlib.h>

// Linked Stack Trace Manager
// 
// Main object, stack_trace_t, represents a stack trace associated
// to some pointer. Stack traces can be linked together to create a
// linked list of stack traces.

// Handle to a stack trace.
typedef struct stack_trace_t stack_trace_t;

// Creates a new stack trace.
// Address is used as an identifier to associate the stack trace with a given pointer.
stack_trace_t* stack_trace_create(void* address, stack_trace_t* next_stack_trace_node);

// Destroy a previously created stack trace.
void stack_trace_destroy(stack_trace_t* source_stack_trace);

// Given an address, seach all stack traces linked to the source stack trace to find the
// first stack trace with the same address as the given address, if such a stack trace exists.
stack_trace_t* stack_trace_find_by_address(stack_trace_t* source_stack_trace, void* address);

// Log the given stack strace as an error to the console.
void stack_trace_log(stack_trace_t* source_stack_trace);
