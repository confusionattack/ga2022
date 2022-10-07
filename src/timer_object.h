#pragma once

// A timer object.
// Supports pause/resume of time.
// Supports scaling time (slowing, speeding up).
// Supports parent-child relationship of time where child inherits parents base time.

#include "heap.h"

#include <stdint.h>

// Handle to a time object.
typedef struct timer_object_t timer_object_t;

// Create a new time object with the defined parent.
// If parent is NULL, use system timer as base time.
timer_object_t* timer_object_create(heap_t* heap, timer_object_t* parent);

// Destroy previously created time object.
void timer_object_destroy(timer_object_t* t);

// Per-frame update for time object.
// Updates current time and delta time.
void timer_object_update(timer_object_t* t);

// Get current time in microseconds.
uint64_t timer_object_get_us(timer_object_t* t);

// Get current time in milliseconds.
uint32_t timer_object_get_ms(timer_object_t* t);

// Get frame time in microseconds.
uint64_t timer_object_get_delta_us(timer_object_t* t);

// Get frame time in milliseconds.
uint32_t timer_object_get_delta_ms(timer_object_t* t);

// Set time scale value.
// Smaller values slow down, larger values speed up.
// A value of 1.0 is normal speed.
void timer_object_set_scale(timer_object_t* t, float s);

// Pause time.
void timer_object_pause(timer_object_t* t);

// Resume previously paused time.
void timer_object_resume(timer_object_t* t);
