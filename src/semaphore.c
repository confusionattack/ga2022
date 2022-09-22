#include "semaphore.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

semaphore_t* semaphore_create(int initial_count, int max_count)
{
	HANDLE semaphore = CreateSemaphore(NULL, initial_count, max_count, NULL);
	return (semaphore_t*)semaphore;
}

void semaphore_destroy(semaphore_t* semaphore)
{
	CloseHandle(semaphore);
}

void semaphore_acquire(semaphore_t* semaphore)
{
	WaitForSingleObject(semaphore, INFINITE);
}

void semaphore_release(semaphore_t* semaphore)
{
	ReleaseSemaphore(semaphore, 1, NULL);
}
