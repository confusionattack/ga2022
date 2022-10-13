#pragma once

#include <stdint.h>

// Threading support.

// Handle to a thread.
typedef struct thread_t thread_t;

// Creates a new thread.
// Thread begins running function with data on return.
thread_t* thread_create(int (*function)(void*), void* data);

// Waits for a thread to complete and destroys it.
// Returns the thread's exit code.
int thread_destroy(thread);

// Puts the calling thread to sleep for the specified number of milliseconds.
// Thread will sleep for *approximately* the specified time.
void thread_sleep(uint32_t ms);
