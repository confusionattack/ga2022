#include "timeofday.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

time_date_time_t timeofday_get()
{
	SYSTEMTIME system_time;
	GetLocalTime(&system_time);

	FILETIME file_time;
	SystemTimeToFileTime(&system_time, &file_time);

	time_date_time_t result =
	{
		.seconds_since_epoch = *(uint64_t*)&file_time / 10000000ULL,
		.day = system_time.wDay,
		.month = system_time.wMonth,
		.year = system_time.wYear,
		.second = system_time.wSecond,
		.minute = system_time.wMinute,
		.hour = system_time.wHour,
	};

	return result;
}
