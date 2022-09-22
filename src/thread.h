// Threading support.

// Handle to a thread.
typedef struct thread_t thread_t;

// Creates a new thread.
// Thread begins running function with data on return.
thread_t* thread_create(int (*function)(void*), void* data);

// Waits for a thread to complete and destroys it.
// Returns the thread's exit code.
int thread_destroy(thread);
