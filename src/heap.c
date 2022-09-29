#include "heap.h"

#include "debug.h"
#include "mutex.h"
#include "tlsf/tlsf.h"

#include <stddef.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Dbghelp.h>

#define MAX_CALLSTACK_DEPTH 64
#define MAX_FUNC_NAME_LENGTH 64

typedef struct arena_t
{
	pool_t pool;
	struct arena_t* next;
} arena_t;

typedef struct alloc_t
{
	void* address;
	size_t size;
	struct alloc_t* next;

	size_t stack_depth;
	void* stack[MAX_CALLSTACK_DEPTH];
} alloc_t;

typedef struct heap_t
{
	tlsf_t tlsf;
	size_t grow_increment;
	arena_t* arena;
	alloc_t* head;
	mutex_t* mutex;
} heap_t;

heap_t* heap_create(size_t grow_increment)
{
	heap_t* heap = VirtualAlloc(NULL, sizeof(heap_t) + tlsf_size(),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!heap)
	{
		debug_print(
			k_print_error,
			"OUT OF MEMORY!\n");
		return NULL;
	}

	heap->mutex = mutex_create();
	heap->grow_increment = grow_increment;
	heap->tlsf = tlsf_create(heap + 1);
	heap->arena = NULL;
	heap->head = NULL;

	return heap;
}

void* heap_alloc(heap_t* heap, size_t size, size_t alignment)
{
	mutex_lock(heap->mutex);

	void* address = tlsf_memalign(heap->tlsf, alignment, size);
	if (!address)
	{
		size_t arena_size =
			__max(heap->grow_increment, size * 2) +
			sizeof(arena_t);
		arena_t* arena = VirtualAlloc(NULL,
			arena_size + tlsf_pool_overhead(),
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!arena)
		{
			debug_print(
				k_print_error,
				"OUT OF MEMORY!\n");
			return NULL;
		}

		arena->pool = tlsf_add_pool(heap->tlsf, arena + 1, arena_size);

		arena->next = heap->arena;
		heap->arena = arena;

		address = tlsf_memalign(heap->tlsf, alignment, size);
	}

	if (address)
	{
		alloc_t* new_alloc = malloc(sizeof(alloc_t));
		new_alloc->address = address;
		new_alloc->size = size;
		new_alloc->next = heap->head;

		int count = CaptureStackBackTrace(0, MAX_CALLSTACK_DEPTH, new_alloc->stack, NULL);
		new_alloc->stack_depth = (count < MAX_CALLSTACK_DEPTH) ? count : MAX_CALLSTACK_DEPTH;

		heap->head = new_alloc;
	}

	mutex_unlock(heap->mutex);

	return address;
}

void heap_free(heap_t* heap, void* address)
{
	mutex_lock(heap->mutex);

	tlsf_free(heap->tlsf, address);

	alloc_t* prev = NULL;
	alloc_t* alloc = heap->head;

	while (alloc)
	{
		if (alloc->address == address)
		{
			if (!prev)
			{
				heap->head = alloc->next;
			}
			else
			{
				prev->next = alloc->next;
			}

			break;
		}

		prev = alloc;
		alloc = alloc->next;
	}

	mutex_unlock(heap->mutex);
}

void detect_and_report_leaks(heap_t* heap)
{
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

	void* process_id = GetCurrentProcess();
	if (!SymInitialize(process_id, NULL, TRUE))
	{
		debug_print(k_print_error, "SymInitialize returned error : %d\n", GetLastError());
	}

	mutex_lock(heap->mutex);

	alloc_t* alloc = heap->head;
	while (alloc)
	{
		debug_print(
			k_print_error,
			"Memory leak of size %d bytes with callstack:\n",
			alloc->size);

		// print the callstack, ignoring the first 7 stack frames since they preceed main
		size_t stack_count = 0;
		for (size_t stack_index = alloc->stack_depth - 7; stack_index >= 1; --stack_index)
		{
			DWORD64 address = (DWORD64)(alloc->stack[stack_index]);

			char symbol_buffer[sizeof(SYMBOL_INFO)+(MAX_FUNC_NAME_LENGTH - 1) * sizeof(TCHAR)];
			SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbol_buffer;
			symbol->MaxNameLen = MAX_FUNC_NAME_LENGTH;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

			SymFromAddr(process_id, address, NULL, symbol);

			debug_print(k_print_error, "[%d] %s\n", stack_count++, symbol->Name);
		}

		alloc = alloc->next;
	}

	mutex_unlock(heap->mutex);

	SymCleanup(process_id);
}

void heap_destroy(heap_t* heap)
{
	detect_and_report_leaks(heap);

	tlsf_destroy(heap->tlsf);

	arena_t* arena = heap->arena;
	while (arena)
	{
		arena_t* next = arena->next;
		VirtualFree(arena, 0, MEM_RELEASE);
		arena = next;
	}

	mutex_destroy(heap->mutex);

	VirtualFree(heap, 0, MEM_RELEASE);
}
