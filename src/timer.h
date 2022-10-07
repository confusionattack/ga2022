#pragma once

// High resolution timer support.

#include <stdint.h>

// Perform one-time initialization of the timer.
void timer_startup();

// Get the number of OS-defined ticks that have elapsed since startup.
uint64_t timer_get_ticks();

// Get the OS-defined tick frequency.
uint64_t timer_get_ticks_per_second();

// Convert a number of OS-defined ticks to microseconds.
uint64_t timer_ticks_to_us(uint64_t t);

// Convert a number of OS-defined ticks to milliseconds.
uint32_t timer_ticks_to_ms(uint64_t t);
