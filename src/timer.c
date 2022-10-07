#include "timer.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static uint64_t s_ticks_start = 0;
static double s_us_per_tick = 0.001;
static double s_ms_per_tick = 0.000001;

void timer_startup()
{
	s_ticks_start = timer_get_ticks();

	uint64_t ticks_per_second = timer_get_ticks_per_second();
	s_us_per_tick = 1000000.0 / ticks_per_second;
	s_ms_per_tick = 1000.0 / ticks_per_second;
}

uint64_t timer_ticks_to_us(uint64_t t)
{
	return (uint64_t)((double)t * s_us_per_tick);
}

uint32_t timer_ticks_to_ms(uint64_t t)
{
	return (uint32_t)((double)t * s_ms_per_tick);
}

uint64_t timer_get_ticks()
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return now.QuadPart - s_ticks_start;
}

uint64_t timer_get_ticks_per_second()
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}
