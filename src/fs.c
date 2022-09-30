#include "fs.h"

#include "debug.h"
#include "event.h"
#include "heap.h"
#include "queue.h"
#include "thread.h"
#include "lz4/lz4.h"

#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CLOSURE_CHAR '\n'

typedef struct fs_t
{
	heap_t* heap;
	queue_t* file_queue;
	thread_t* file_thread;
	queue_t* compression_queue;
	thread_t* compression_thread;
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
	void* tmp_buffer;
	size_t size;
	size_t tmp_size;
	event_t* done;
	int result;
} fs_work_t;

static int file_thread_func(void* user);
static int compression_thread_func(void* user);

fs_t* fs_create(heap_t* heap, int queue_capacity)
{
	fs_t* fs = heap_alloc(heap, sizeof(fs_t), 8);
	fs->heap = heap;
	fs->file_queue = queue_create(heap, queue_capacity);
	fs->file_thread = thread_create(file_thread_func, fs);
	fs->compression_queue = queue_create(heap, queue_capacity);
	fs->compression_thread = thread_create(compression_thread_func, fs);
	return fs;
}

void fs_destroy(fs_t* fs)
{
	queue_push(fs->file_queue, NULL);
	thread_destroy(fs->file_thread);
	queue_destroy(fs->file_queue);
	
	queue_push(fs->compression_queue, NULL);
	thread_destroy(fs->compression_thread);
	queue_destroy(fs->compression_queue);

	heap_free(fs->heap, fs);
}

fs_work_t* fs_read(fs_t* fs, const char* path, heap_t* heap, bool null_terminate, bool use_compression)
{
	fs_work_t* work = heap_alloc(fs->heap, sizeof(fs_work_t), 8);
	work->heap = heap;
	work->op = k_fs_work_op_read;
	strcpy_s(work->path, sizeof(work->path), path);
	work->buffer = NULL;
	work->tmp_buffer = NULL;
	work->size = 0;
	work->tmp_size = 0;
	work->done = event_create();
	work->result = 0;
	work->null_terminate = null_terminate;
	work->use_compression = use_compression;
	queue_push(fs->file_queue, work);
	return work;
}

fs_work_t* fs_write(fs_t* fs, const char* path, const void* buffer, size_t size, bool use_compression)
{
	fs_work_t* work = heap_alloc(fs->heap, sizeof(fs_work_t), 8);
	work->heap = fs->heap;
	work->op = k_fs_work_op_write;
	strcpy_s(work->path, sizeof(work->path), path);
	work->buffer = (void*)buffer;
	work->tmp_buffer = NULL;
	work->size = size;
	work->tmp_size = 0;
	work->done = event_create();
	work->result = 0;
	work->null_terminate = false;
	work->use_compression = use_compression;

	if (use_compression)
	{
		queue_push(fs->compression_queue, work);
	}
	else
	{
		queue_push(fs->file_queue, work);
	}

	return work;
}

bool fs_work_is_done(fs_work_t* work)
{
	return work ? event_is_raised(work->done) : true;
}

void fs_work_wait(fs_work_t* work)
{
	if (work)
	{
		event_wait(work->done);
	}
}

int fs_work_get_result(fs_work_t* work)
{
	fs_work_wait(work);
	return work ? work->result : -1;
}

void* fs_work_get_buffer(fs_work_t* work)
{
	fs_work_wait(work);
	return work ? work->buffer : NULL;
}

size_t fs_work_get_size(fs_work_t* work)
{
	fs_work_wait(work);
	return work ? work->size : 0;
}

void fs_work_destroy(fs_work_t* work)
{
	if (work)
	{
		event_wait(work->done);
		event_destroy(work->done);
		heap_free(work->heap, work);
	}
}

static void file_read(fs_t* fs, fs_work_t* work)
{
	wchar_t wide_path[1024];
	if (MultiByteToWideChar(CP_UTF8, 0, work->path, -1, wide_path, sizeof(wide_path)) <= 0)
	{
		work->result = -1;
		return;
	}

	HANDLE handle = CreateFile(wide_path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		work->result = GetLastError();
		return;
	}

	if (!GetFileSizeEx(handle, (PLARGE_INTEGER)&work->size))
	{
		work->result = GetLastError();
		CloseHandle(handle);
		return;
	}

	work->buffer = heap_alloc(work->heap, (work->null_terminate && !work->use_compression) ? work->size + 1 : work->size, 8);

	DWORD bytes_read = 0;
	if (!ReadFile(handle, work->buffer, (DWORD)work->size, &bytes_read, NULL))
	{
		work->result = GetLastError();
		CloseHandle(handle);
		return;
	}

	work->size = bytes_read;
	// Should not null terminate if the data read is compressed as that will mess with decompression
	if (work->null_terminate && !work->use_compression)
	{
		((char*)work->buffer)[bytes_read] = 0;
	}

	CloseHandle(handle);

	if (work->use_compression)
	{
		queue_push(fs->compression_queue, work);
	}
	else
	{
		event_signal(work->done);
	}
}

static void file_compress_read(fs_work_t* work)
{
	size_t buffer_size = 0;
	int digits = 0;
	// Read the size of the uncompressed buffer off the end of the work buffer
	while (((char*)work->buffer)[work->size - digits - 1] != CLOSURE_CHAR)
	{
		int tmp = ((int)((char*)work->buffer)[work->size - digits - 1] - 48);
		buffer_size = (buffer_size * 10) + tmp;
		digits++;
	}
	digits++;

	void* dst_buffer = heap_alloc(work->heap, buffer_size, 8);
	int decompressed_size = LZ4_decompress_safe(work->buffer, dst_buffer, (int)(work->size - digits), (int) buffer_size);

	if (decompressed_size < 0)
	{
		debug_print(k_print_error, "File decompression failed\n");
	}

	// Free buffer with compressed data
	heap_free(work->heap, work->buffer);

	work->buffer = dst_buffer;
	work->size = decompressed_size;

	// Null terminate the buffer if requested
	if (work->null_terminate)
	{
		((char*)work->buffer)[work->size] = 0;
	}

	event_signal(work->done);
}

static void file_write(fs_work_t* work)
{
	wchar_t wide_path[1024];
	if (MultiByteToWideChar(CP_UTF8, 0, work->path, -1, wide_path, sizeof(wide_path)) <= 0)
	{
		work->result = -1;
		return;
	}

	HANDLE handle = CreateFile(wide_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		work->result = GetLastError();
		return;
	}

	DWORD bytes_written = 0;
	if (!WriteFile(handle, work->buffer, (DWORD)work->size, &bytes_written, NULL))
	{
		work->result = GetLastError();
		CloseHandle(handle);
		return;
	}

	work->size = bytes_written;	

	// Append the uncompressed buffer's size to the end of the file if compressed
	if (work->use_compression)
	{
		size_t size = work->tmp_size;
		char* size_buffer = heap_alloc(work->heap, 1, 1);

		*size_buffer = CLOSURE_CHAR;

		if (!WriteFile(handle, size_buffer, 1, NULL, NULL))
		{
			work->result = GetLastError();
			CloseHandle(handle);
			return;
		}

		while (size > 0) {
			int num = size % 10;
			*size_buffer = (char)(size % 10 + 48);

			if (!WriteFile(handle, size_buffer, 1, NULL, NULL))
			{
				work->result = GetLastError();
				CloseHandle(handle);
				return;
			}

			size /= 10;
		}

		heap_free(work->heap, size_buffer);
	}

	CloseHandle(handle);

	if (work->use_compression)
	{
		// Must free work buffer as it was a temporary buffer used to hold the compressed data
		heap_free(work->heap, work->buffer);

		// Set buffer and size to pre-compressed values
		work->buffer = work->tmp_buffer;
		work->tmp_buffer = NULL;
		work->size = work->tmp_size;
		work->tmp_size = 0;
	}

	event_signal(work->done);
}

static void file_compress_write(fs_t* fs, fs_work_t* work)
{
	int dst_size = LZ4_compressBound((int)work->size);
	void* dst = heap_alloc(work->heap, dst_size, 8);
	int compressed_size = LZ4_compress_default(work->buffer, dst, (int)work->size, dst_size);

	if (compressed_size < 0)
	{
		debug_print(k_print_error, "File compression failed\n");
	}

	work->tmp_buffer = work->buffer;
	work->tmp_size = work->size;

	work->buffer = dst;
	work->size = compressed_size;

	queue_push(fs->file_queue, work);
}

static int file_thread_func(void* user)
{
	fs_t* fs = user;
	while (true)
	{
		fs_work_t* work = queue_pop(fs->file_queue);
		if (work == NULL)
		{
			break;
		}
		
		switch (work->op)
		{
		case k_fs_work_op_read:
			file_read(fs, work);
			break;
		case k_fs_work_op_write:
			file_write(work);
			break;
		}
	}
	return 0;
}

static int compression_thread_func(void* user)
{
	fs_t* fs = user;
	while (true)
	{
		fs_work_t* work = queue_pop(fs->compression_queue);
		if (work == NULL)
		{
			break;
		}

		switch (work->op)
		{
		case k_fs_work_op_read:
			file_compress_read(work);
			break;
		case k_fs_work_op_write:
			file_compress_write(fs, work);
			break;
		}
	}
	return 0;
}
