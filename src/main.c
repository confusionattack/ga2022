#include "heap.h"
#include "wm.h"

#include <stdio.h>

void heap_test()
{
	heap_t* heap = heap_create(4096);
	void* leak = heap_alloc(heap, 100, 8);
	void* not_leak = heap_alloc(heap, 200, 8);
	heap_free(heap, not_leak);
	heap_destroy(heap);
}

int main(int argc, const char* argv[])
{
	heap_test();

	heap_t* heap = heap_create(2 * 1024 * 1024);
	wm_window_t* window = wm_create(heap);

	// THIS IS THE MAIN LOOP!
	while (!wm_pump(window))
	{
		int x, y;
		wm_get_mouse_move(window, &x, &y);
		printf("MOUSE mask=%x move=%dx%x\n",
			wm_get_mouse_mask(window),
			x, y);
	}

	wm_destroy(window);
	heap_destroy(heap);

	return 0;
}
