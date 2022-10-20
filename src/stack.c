#include "stack.h"

#include "atomic.h"
#include "heap.h"
#include "semaphore.h"

typedef struct stack_t
{
	heap_t* heap;
	semaphore_t* used_items;
	semaphore_t* free_items;
	void** items;
	int capacity;
	int size;
} stack_t;

stack_t* stack_create(heap_t* heap, int capacity)
{
	stack_t* stack = heap_alloc(heap, sizeof(stack_t), 8);
	stack->items = heap_alloc(heap, sizeof(void*) * capacity, 8);
	stack->used_items = semaphore_create(0, capacity);
	stack->free_items = semaphore_create(capacity, capacity);
	stack->heap = heap;
	stack->capacity = capacity;
	stack->size = 0;
	return stack;
}

void stack_destroy(stack_t* stack)
{
	semaphore_destroy(stack->used_items);
	semaphore_destroy(stack->free_items);
	heap_free(stack->heap, stack->items);
	heap_free(stack->heap, stack);
}

void stack_push(stack_t* stack, void* item)
{
	semaphore_acquire(stack->free_items);
	stack->items[stack->size] = item;
	atomic_increment(&stack->size);
	semaphore_release(stack->used_items);
}

void* stack_pop(stack_t* stack)
{
	semaphore_acquire(stack->used_items);
	atomic_decrement(&stack->size);
	void* item = stack->items[stack->size];
	semaphore_release(stack->free_items);
	return item;
}
