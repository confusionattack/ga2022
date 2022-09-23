#include "fs.h"

#include "event.h"
#include "heap.h"
#include "queue.h"
#include "thread.h"

#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct fs_t
{
	heap_t* heap;
	queue_t* file_queue;
	thread_t* file_thread;
} fs_t;

typedef enum fs_work_op_t
{
	k_fs_work_op_read,
	k_fs_work_op_write,
} fs_work_op_t;

typedef struct fs_work_t
{
	heap_t* heap;
	fs_work_op_t op;
	char path[1024];
	bool null_terminate;
	bool use_compression;
	void* buffer;
	size_t size;
	event_t* done;
	int result;
} fs_work_t;

static int file_thread_func(void* user);

fs_t* fs_create(heap_t* heap, int queue_capacity)
{
	fs_t* fs = heap_alloc(heap, sizeof(fs_t), 8);
	fs->heap = heap;
	fs->file_queue = queue_create(heap, queue_capacity);
	fs->file_thread = thread_create(file_thread_func, fs);
	return fs;
}

void fs_destroy(fs_t* fs)
{
	queue_push(fs->file_queue, NULL);
	thread_destroy(fs->file_thread);
	queue_destroy(fs->file_queue);
	heap_free(fs->heap, fs);
}

fs_work_t* fs_read(fs_t* fs, const char* path, bool null_terminate, bool use_compression)
{
	fs_work_t* item = heap_alloc(fs->heap, sizeof(fs_work_t), 8);
	item->heap = fs->heap;
	item->op = k_fs_work_op_read;
	strcpy_s(item->path, sizeof(item->path), path);
	item->buffer = NULL;
	item->size = 0;
	item->done = event_create();
	item->result = 0;
	item->null_terminate = null_terminate;
	item->use_compression = use_compression;
	queue_push(fs->file_queue, item);
	return item;
}

fs_work_t* fs_write(fs_t* fs, const char* path, const void* buffer, size_t size, bool use_compression)
{
	fs_work_t* item = heap_alloc(fs->heap, sizeof(fs_work_t), 8);
	item->heap = fs->heap;
	item->op = k_fs_work_op_write;
	strcpy_s(item->path, sizeof(item->path), path);
	item->buffer = (void*)buffer;
	item->size = size;
	item->done = event_create();
	item->result = 0;
	item->null_terminate = false;
	item->use_compression = use_compression;

	if (use_compression)
	{
		// HOMEWORK 2: Queue file write work on compression queue!
	}
	else
	{
		queue_push(fs->file_queue, item);
	}

	return item;
}

bool fs_work_is_done(fs_work_t* work)
{
	return event_is_raised(work->done);
}

void fs_work_wait(fs_work_t* work)
{
	event_wait(work->done);
}

int fs_work_get_result(fs_work_t* work)
{
	event_wait(work->done);
	return work->result;
}

void* fs_work_get_buffer(fs_work_t* work)
{
	event_wait(work->done);
	return work->buffer;
}

size_t fs_work_get_size(fs_work_t* work)
{
	event_wait(work->done);
	return work->size;
}

void fs_work_destroy(fs_work_t* work)
{
	event_wait(work->done);
	event_destroy(work->done);
	if (work->op == k_fs_work_op_read)
	{
		heap_free(work->heap, work->buffer);
	}
	heap_free(work->heap, work);
}

static void file_read(fs_work_t* item)
{
	wchar_t wide_path[1024];
	if (MultiByteToWideChar(CP_UTF8, 0, item->path, -1, wide_path, sizeof(wide_path)) <= 0)
	{
		item->result = -1;
		return;
	}

	HANDLE handle = CreateFile(wide_path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		item->result = GetLastError();
		return;
	}

	if (!GetFileSizeEx(handle, (PLARGE_INTEGER)&item->size))
	{
		item->result = GetLastError();
		CloseHandle(handle);
		return;
	}

	item->buffer = heap_alloc(item->heap, item->null_terminate ? item->size + 1 : item->size, 8);

	DWORD bytes_read = 0;
	if (!ReadFile(handle, item->buffer, (DWORD)item->size, &bytes_read, NULL))
	{
		item->result = GetLastError();
		CloseHandle(handle);
		return;
	}

	item->size = bytes_read;
	if (item->null_terminate)
	{
		((char*)item->buffer)[bytes_read] = 0;
	}

	CloseHandle(handle);

	if (item->use_compression)
	{
		// HOMEWORK 2: Queue file write work on decompression queue!
	}
	else
	{
		event_signal(item->done);
	}
}

static void file_write(fs_work_t* item)
{
	wchar_t wide_path[1024];
	if (MultiByteToWideChar(CP_UTF8, 0, item->path, -1, wide_path, sizeof(wide_path)) <= 0)
	{
		item->result = -1;
		return;
	}

	HANDLE handle = CreateFile(wide_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		item->result = GetLastError();
		return;
	}

	DWORD bytes_written = 0;
	if (!WriteFile(handle, item->buffer, (DWORD)item->size, &bytes_written, NULL))
	{
		item->result = GetLastError();
		CloseHandle(handle);
		return;
	}

	item->size = bytes_written;

	CloseHandle(handle);

	event_signal(item->done);
}

static int file_thread_func(void* user)
{
	fs_t* fs = user;
	while (true)
	{
		fs_work_t* item = queue_pop(fs->file_queue);
		if (item == NULL)
		{
			break;
		}
		
		switch (item->op)
		{
		case k_fs_work_op_read:
			file_read(item);
			break;
		case k_fs_work_op_write:
			file_write(item);
			break;
		}
	}
	return 0;
}
