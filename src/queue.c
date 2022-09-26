#include "atomic.h"
#include "heap.h"
#include "semaphore.h"

typedef struct queue_t
{
	heap_t* heap;
	semaphore_t* used_items;
	semaphore_t* free_items;
	void** items;
	int capacity;
	int head_index;
	int tail_index;
} queue_t;

queue_t* queue_create(heap_t* heap, int capacity)
{
	queue_t* queue = heap_alloc(heap, sizeof(queue_t), 8);
	queue->items = heap_alloc(heap, sizeof(void*) * capacity, 8);
	queue->used_items = semaphore_create(0, capacity);
	queue->free_items = semaphore_create(capacity, capacity);
	queue->heap = heap;
	queue->capacity = capacity;
	queue->head_index = 0;
	queue->tail_index = 0;
	return queue;
}

void queue_destroy(queue_t* queue)
{
	semaphore_destroy(queue->used_items);
	semaphore_destroy(queue->free_items);
	heap_free(queue->heap, queue->items);
	heap_free(queue->heap, queue);
}

void queue_push(queue_t* queue, void* item)
{
	semaphore_acquire(queue->free_items);
	int index = atomic_increment(&queue->tail_index) % queue->capacity;
	queue->items[index] = item;
	semaphore_release(queue->used_items);
}

void* queue_pop(queue_t* queue)
{
	semaphore_acquire(queue->used_items);
	int index = atomic_increment(&queue->head_index) % queue->capacity;
	void* item = queue->items[index];
	semaphore_release(queue->free_items);
	return item;
}
