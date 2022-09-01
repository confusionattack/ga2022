#include "wm.h"

#include <stdio.h>

int main(int argc, const char* argv[])
{
	wm_window_t* window = wm_create();

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

	return 0;
}
