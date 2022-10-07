#pragma once

// Counting semaphore thread synchronization

// Handle to a semaphore.
typedef struct semaphore_t semaphore_t;

// Creates a new semaphore.
semaphore_t* semaphore_create(int initial_count, int max_count);

// Destroys a previously created semaphore.
void semaphore_destroy(semaphore_t* semaphore);

// Lowers the semaphore count by one.
// If the semaphore count is zero, blocks until another thread releases.
void semaphore_acquire(semaphore_t* semaphore);

// Raises the semaphore count by one.
void semaphore_release(semaphore_t* semaphore);
