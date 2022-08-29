// c:> ga2022.exe foo bar

#include "wm.h"

#include <stdio.h>

int main(int argc, const char* argv[])
{
	wm_window_t* window = wm_create();

	// THIS IS THE MAIN LOOP!
	while (!wm_pump(window))
	{
		printf("FRAME!\n");
	}

	wm_destroy(window);

	return 0;
}
