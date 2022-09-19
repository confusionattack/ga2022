#include "atomic.h"
#include "debug.h"
#include "heap.h"
#include "mutex.h"
#include "thread.h"
#include "wm.h"

#include <stdio.h>

#include <windows.h>

static void homework1_test();

int main(int argc, const char* argv[])
{
	debug_install_exception_handler();

	homework1_test();

	extern void lecture7_thread_test();
	lecture7_thread_test();

	extern void lecture7_queue_test();
	lecture7_queue_test();

	debug_set_print_mask(k_print_warning | k_print_error);

	heap_t* heap = heap_create(2 * 1024 * 1024);
	wm_window_t* window = wm_create(heap);

	// THIS IS THE MAIN LOOP!
	while (!wm_pump(window))
	{
		int x, y;
		wm_get_mouse_move(window, &x, &y);

		uint32_t mask = wm_get_mouse_mask(window);

		debug_print(
			k_print_info,
			"MOUSE mask=%x move=%dx%d\n",
			mask,
			x, y);
	}

	wm_destroy(window);
	heap_destroy(heap);

	return 0;
}

static void* homework1_allocate_1(heap_t* heap)
{
	return heap_alloc(heap, 16 * 1024, 8);
}

static void* homework1_allocate_2(heap_t* heap)
{
	return heap_alloc(heap, 256, 8);
}

static void* homework1_allocate_3(heap_t* heap)
{
	return heap_alloc(heap, 32 * 1024, 8);
}

static void homework1_test()
{
	heap_t* heap = heap_create(4096);
	void* block1 = homework1_allocate_1(heap);
	/*leaked*/ homework1_allocate_2(heap);
	/*leaked*/ homework1_allocate_3(heap);
	heap_free(heap, block1);
	heap_destroy(heap);
}
