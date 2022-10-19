#pragma once

// Hash Table for Chrome Tracing
// 
// Main object, trace_hash_table_t, represents a collection of stacks
// of duration events. As each thread has needs its own stack of duration 
// events, the keys of each entry in trace_hash_table_t should be a thread id.

typedef struct heap_t heap_t;
typedef struct trace_duration_t trace_duration_t;

// Handle to a trace hash table.
typedef struct trace_hash_table_t trace_hash_table_t;

// Creates a new trace hash table.
// Capacity is the maximum number of duration events that can ever be in
// the hash table at any given time.
trace_hash_table_t* trace_hash_table_create(heap_t* heap, int capacity);

// Destroy a previously trace hash table.
// All entries that are left on the stacks are freed.
void trace_hash_table_destroy(trace_hash_table_t* hash_table);

// Gets the total number of durations in the hash table. 
int trace_hash_table_size(trace_hash_table_t* hash_table);

// Push an item onto the stack associated to the given key.
// If the table's capacity is reached or the stack is full, 
// this function does nothing.
void trace_hash_table_push(trace_hash_table_t* hash_table, int key, trace_duration_t* value);

// Pop a duration event off the stack (LIFO order) associated to the given key 
// and sets the end time slice of the trace duration that was popped off.
// If this key does not have a stack associated with it or if the stack is empty, 
// this function does nothing.
void trace_hash_table_pop_off_stack(trace_hash_table_t* hash_table, int key);

// Pops a duration event off of the queue (FIFO order). 
// If the queue is empty, NULL is returned.
// IMPORTANT: This function should only be called if nothing else is expected
// to be pushed to the hash table.
trace_duration_t* trace_hash_table_pop_off_events_queue(trace_hash_table_t* hash_table);
