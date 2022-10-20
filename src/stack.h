#pragma once

// Thread-safe Stack container

// Handle to a thread-safe stack.
typedef struct stack_t stack_t;

typedef struct heap_t heap_t;

// Create a stack with the defined capacity.
stack_t* stack_create(heap_t* heap, int capacity);

// Destroy a previously created stack.
void stack_destroy(stack_t* stack);

// Push an item onto a stack.
// If the stack is full, blocks until space is available.
// Safe for multiple threads to push at the same time.
void stack_push(stack_t* stack, void* item);

// Pop an item off a stack (LIFO order).
// If the stack is empty, blocks until an item is avaiable.
// Safe for multiple threads to pop at the same time.
void* stack_pop(stack_t* stack);
