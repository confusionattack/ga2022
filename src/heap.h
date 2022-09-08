#include <stdlib.h>

typedef struct heap_t heap_t;

heap_t* heap_create(size_t grow_increment);
void* heap_alloc(heap_t* heap, size_t size, size_t alignment);
void heap_free(heap_t* heap, void* address);
void heap_destroy(heap_t* heap);
