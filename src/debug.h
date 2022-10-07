#pragma once

#include <stdint.h>

// Debugging Support

// Flags for debug_print().
typedef enum debug_print_t
{
	k_print_info = 1 << 0,
	k_print_warning = 1 << 1,
	k_print_error = 1 << 2,
} debug_print_t;

// Install unhandled exception handler.
// When unhandled exceptions are caught, will log an error and capture a memory dump.
void debug_install_exception_handler();

// Set mask of which types of prints will actually fire.
// See the debug_print().
void debug_set_print_mask(uint32_t mask);

// Log a message to the console.
// Message may be dropped if type is not in the active mask.
// See debug_set_print_mask.
void debug_print(uint32_t type, _Printf_format_string_ const char* format, ...);

// Capture a list of addresses that make up the current function callstack.
// On return, stack contains at most stack_capacity addresses.
// The number of addresses captured is the return value.
int debug_backtrace(void** stack, int stack_capacity);
