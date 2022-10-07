#pragma once

// Date and time support.

#include <stdint.h>

// Defines a structure containing date and time information.
typedef struct time_date_time_t
{
	uint64_t seconds_since_epoch;
	union
	{
		uint32_t packed_date;
		struct
		{
			uint32_t day : 5;
			uint32_t month : 4;
			uint32_t year : 23;
		};
	};

	union
	{
		uint32_t packed_time;
		struct
		{
			uint32_t second : 6;
			uint32_t minute : 6;
			uint32_t hour : 5;
		};
	};
} time_date_time_t;

// Get the current, local date and time.
time_date_time_t timeofday_get();
