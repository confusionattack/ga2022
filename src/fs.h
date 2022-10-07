#pragma once

#include <stdbool.h>

// Asynchronous read/write file system.

// Handle to file system.
typedef struct fs_t fs_t;

// Handle to file work.
typedef struct fs_work_t fs_work_t;

typedef struct heap_t heap_t;

// Create a new file system.
// Provided heap will be used to allocate space for queue and work buffers.
// Provided queue size defines number of in-flight file operations.
fs_t* fs_create(heap_t* heap, int queue_capacity);

// Destroy a previously created file system.
void fs_destroy(fs_t* fs);

// Queue a file read.
// File at the specified path will be read in full.
// Memory for the file will be allocated out of the provided heap.
// It is the calls responsibility to free the memory allocated!
// Returns a work object.
fs_work_t* fs_read(fs_t* fs, const char* path, heap_t* heap, bool null_terminate, bool use_compression);

// Queue a file write.
// File at the specified path will be written in full.
// Returns a work object.
fs_work_t* fs_write(fs_t* fs, const char* path, const void* buffer, size_t size, bool use_compression);

// If true, the file work is complete.
bool fs_work_is_done(fs_work_t* work);

// Block for the file work to complete.
void fs_work_wait(fs_work_t* work);

// Get the error code for the file work.
// A value of zero generally indicates success.
int fs_work_get_result(fs_work_t* work);

// Get the buffer associated with the file operation.
void* fs_work_get_buffer(fs_work_t* work);

// Get the size associated with the file operation.
size_t fs_work_get_size(fs_work_t* work);

// Free a file work object.
void fs_work_destroy(fs_work_t* work);
