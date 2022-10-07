#pragma once

// Thread-safe Queue container

// Handle to a thread-safe queue.
typedef struct queue_t queue_t;

typedef struct heap_t heap_t;

// Create a queue with the defined capacity.
queue_t* queue_create(heap_t* heap, int capacity);

// Destroy a previously created queue.
void queue_destroy(queue_t* queue);

// Push an item onto a queue.
// If the queue is full, blocks until space is available.
// Safe for multiple threads to push at the same time.
void queue_push(queue_t* queue, void* item);

// Pop an item off a queue (FIFO order).
// If the queue is empty, blocks until an item is avaiable.
// Safe for multiple threads to pop at the same time.
void* queue_pop(queue_t* queue);
