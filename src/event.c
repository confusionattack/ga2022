#include "event.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

event_t* event_create()
{
	HANDLE evt = CreateEvent(NULL, TRUE, FALSE, NULL);
	return (event_t*)evt;
}

void event_destroy(event_t* event)
{
	CloseHandle(event);
}

void event_signal(event_t* event)
{
	SetEvent(event);
}

void event_wait(event_t* event)
{
	WaitForSingleObject(event, INFINITE);
}
