#pragma once

// Recursive mutex thread synchronization

// Handle to a mutex.
typedef struct mutex_t mutex_t;

// Creates a new mutex.
mutex_t* mutex_create();

// Destroys a previously created mutex.
void mutex_destroy(mutex_t* mutex);

// Locks a mutex. May block if another thread unlocks it.
// If a thread locks a mutex multiple times, it must be unlocked
// multiple times.
void mutex_lock(mutex_t* mutex);

// Unlocks a mutex.
void mutex_unlock(mutex_t* mutex);
