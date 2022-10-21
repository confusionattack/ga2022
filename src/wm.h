#include <stdbool.h>
#include <stdint.h>

// Window Manager
// 
// Main object is wm_window_t to represents a single OS-level window.
// Window should be pumped every frame.
// After pumping a window can be queried for user input.

// Handle to a window.
typedef struct wm_window_t wm_window_t;

typedef struct heap_t heap_t;

// Mouse buttons. See wm_get_mouse_mask().
enum
{
	k_mouse_button_left = 1 << 0,
	k_mouse_button_right = 1 << 1,
	k_mouse_button_middle = 1 << 2,
};

// Keyboard keys. See wm_get_key_mask().
enum
{
	k_key_up = 1 << 0,
	k_key_down = 1 << 1,
	k_key_left = 1 << 2,
	k_key_right = 1 << 3,
};


// Creates a new window. Must be destroyed with wm_destroy().
// Returns NULL on failure, otherwise a new window.
wm_window_t* wm_create(heap_t* heap);

// Destroy a previously created window.
void wm_destroy(wm_window_t* window);

// Pump the messages for a window.
// This will refresh the mouse and key state on the window.
bool wm_pump(wm_window_t* window);

// Get a mask of all mouse buttons current held.
uint32_t wm_get_mouse_mask(wm_window_t* window);

// Get a mask of all keyboard keys current held.
uint32_t wm_get_key_mask(wm_window_t* window);

// Get relative mouse movement in x and y.
void wm_get_mouse_move(wm_window_t* window, int* x, int* y);

// Get the raw OS window object.
void* wm_get_raw_window(wm_window_t* window);
