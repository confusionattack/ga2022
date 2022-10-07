#include "timer_object.h"

#include "timer.h"

#include <stdbool.h>

typedef struct timer_object_t
{
	heap_t* heap;
	uint64_t current_ticks;
	uint64_t delta_ticks;
	timer_object_t* parent;
	uint64_t bias_ticks;
	double scale;
	bool paused;
} timer_object_t;

timer_object_t* timer_object_create(heap_t* heap, timer_object_t* parent)
{
	timer_object_t* t = heap_alloc(heap, sizeof(timer_object_t), 8);
	t->heap = heap;
	t->current_ticks = 0;
	t->delta_ticks = 0;
	t->parent = parent;
	t->bias_ticks = parent ? parent->current_ticks : timer_get_ticks();
	t->scale = 1.0;
	t->paused = false;
	return t;
}

void timer_object_destroy(timer_object_t* t)
{
	heap_free(t->heap, t);
}

void timer_object_update(timer_object_t* t)
{
	if (!t->paused)
	{
		uint64_t parent_ticks = t->parent ? t->parent->current_ticks : timer_get_ticks();
		t->delta_ticks = (uint64_t)((parent_ticks - t->bias_ticks) * t->scale);
		t->current_ticks += t->delta_ticks;
		t->bias_ticks = parent_ticks;
	}
}

uint64_t timer_object_get_us(timer_object_t* t)
{
	return timer_ticks_to_us(t->current_ticks);
}

uint32_t timer_object_get_ms(timer_object_t* t)
{
	return timer_ticks_to_ms(t->current_ticks);
}

uint64_t timer_object_get_delta_us(timer_object_t* t)
{
	return timer_ticks_to_us(t->delta_ticks);
}

uint32_t timer_object_get_delta_ms(timer_object_t* t)
{
	return timer_ticks_to_ms(t->delta_ticks);
}

void timer_object_set_scale(timer_object_t* t, float s)
{
	t->scale = s;
}

void timer_object_pause(timer_object_t* t)
{
	t->paused = true;
}

void timer_object_resume(timer_object_t* t)
{
	if (t->paused)
	{
		t->bias_ticks = t->parent ? t->parent->current_ticks : timer_get_ticks();
		t->paused = false;
	}
}
