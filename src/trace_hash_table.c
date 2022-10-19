#include "trace_hash_table.h"

#include "atomic.h"
#include "heap.h"
#include "mutex.h"
#include "stack.h"
#include "trace.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_STACK_LENGTH 30

typedef struct trace_hash_table_entry_t trace_hash_table_entry_t;
void trace_hash_table_entry_destroy(trace_hash_table_entry_t* entry);

typedef struct trace_hash_table_entry_t
{
	heap_t* heap;
	int key;
	stack_t* value;
	int size;
	trace_hash_table_entry_t* next;
} trace_hash_table_entry_t;

typedef struct trace_hash_table_t
{
	heap_t* heap;
	mutex_t* lock;
	int capacity;
	int size;
	trace_hash_table_entry_t** table;
} trace_hash_table_t;


trace_hash_table_t* trace_hash_table_create(heap_t* heap, int capacity)
{
	trace_hash_table_t* hash_table = heap_alloc(heap, sizeof(trace_hash_table_t), 8);
	hash_table->heap = heap;
	hash_table->capacity = capacity;
	hash_table->size = 0;
	hash_table->table = heap_alloc(heap, sizeof(trace_hash_table_entry_t*) * capacity, 8);
	hash_table->lock = mutex_create();
	// null each entry of the table
	for (int i = 0; i < capacity; i++)
	{
		hash_table->table[i] = NULL;
	}
	return hash_table;
}

void trace_hash_table_destroy(trace_hash_table_t* hash_table)
{
	mutex_lock(hash_table->lock);
	// Free all entries in table
	for (int i = 0; i < hash_table->capacity; i++)
	{
		if (hash_table->table[i] != NULL)
		{
			trace_hash_table_entry_destroy(hash_table->table[i]);
		}
	}
	mutex_unlock(hash_table->lock);
	mutex_destroy(hash_table->lock);
	heap_free(hash_table->heap, hash_table->table);
	heap_free(hash_table->heap, hash_table);
}

trace_hash_table_entry_t* trace_hash_table_entry_create(heap_t* heap, int key)
{
	trace_hash_table_entry_t* entry = heap_alloc(heap, sizeof(trace_hash_table_entry_t), 8);
	entry->heap = heap;
	entry->key = key;
	entry->value = stack_create(entry->heap, MAX_STACK_LENGTH);
	entry->next = NULL;
	return entry;
}

void trace_hash_table_entry_destroy(trace_hash_table_entry_t* entry)
{
	while (entry->size > 0)
	{
		trace_duration_destroy(stack_pop(entry->value));
		atomic_decrement(&entry->size);
	}
	stack_destroy(entry->value);
	if (entry->next != NULL)
	{
		trace_hash_table_entry_destroy(entry->next);
		entry->next = NULL;
	}
	heap_free(entry->heap, entry);
}

int trace_hash_table_hash(trace_hash_table_t* hash_table, int key)
{
	return key % hash_table->capacity;
}

int trace_hash_table_size(trace_hash_table_t* hash_table)
{
	mutex_lock(hash_table->lock);
	int size = hash_table->size;
	mutex_unlock(hash_table->lock);
	return size;
}

void trace_hash_table_push(trace_hash_table_t* hash_table, int key, trace_duration_t* value)
{
	//int key = GetCurrentThreadId();
	int index = trace_hash_table_hash(hash_table, key);

	mutex_lock(hash_table->lock);
	if (hash_table->size == hash_table->capacity)
	{
		mutex_unlock(hash_table->lock);
		return;
	}

	trace_hash_table_entry_t* entry = hash_table->table[index];

	if (entry == NULL)
	{
		hash_table->table[index] = trace_hash_table_entry_create(hash_table->heap, key);
		stack_push(hash_table->table[index]->value, value);
		atomic_increment(&hash_table->table[index]->size);
		atomic_increment(&hash_table->size);
		mutex_unlock(hash_table->lock);
		return;
	}

	trace_hash_table_entry_t* previous_entry = NULL;
	while (entry != NULL)
	{
		if (entry->key == key)
		{
			if (entry->size < MAX_STACK_LENGTH)
			{
				stack_push(entry->value, value);
				atomic_increment(&entry->size);
				atomic_increment(&hash_table->size);
			}
			mutex_unlock(hash_table->lock);
			return;
		}

		previous_entry = entry;
		entry = previous_entry->next;
	}

	previous_entry->next = trace_hash_table_entry_create(hash_table->heap, key);
	entry = previous_entry->next;

	stack_push(entry->value, value);
	atomic_increment(&entry->size);
	atomic_increment(&hash_table->size);

	mutex_unlock(hash_table->lock);
}

trace_duration_t* trace_hash_table_pop(trace_hash_table_t* hash_table, int key)
{
	//int key = GetCurrentThreadId();
	int index = trace_hash_table_hash(hash_table, key);

	mutex_lock(hash_table->lock);
	trace_hash_table_entry_t* entry = hash_table->table[index];

	if (entry == NULL)
	{
		mutex_unlock(hash_table->lock);
		return NULL;
	}

	while (entry != NULL)
	{
		if (entry->key == key)
		{
			if (entry->size == 0)
			{
				mutex_unlock(hash_table->lock);
				return NULL;
			}
			trace_duration_t* duration = stack_pop(entry->value);
			atomic_decrement(&entry->size);
			atomic_decrement(&hash_table->size);
			mutex_unlock(hash_table->lock);
			return duration;
		}
		entry = entry->next;
	}

	mutex_unlock(hash_table->lock);
	return NULL;
}




#include <stdio.h>		// FOR DEBUGGING ONLY ==============================================
void hash_table_dump(trace_hash_table_t* table)
{
	mutex_lock(table->lock);
	printf("TABLE: SIZE = %d / %d\n", table->size, table->capacity);
	for (int i = 0; i < table->capacity; i++)
	{
		trace_hash_table_entry_t* entry = table->table[i];

		printf("%d: \n", i);
		if (entry == NULL)
		{
			continue;
		}

		for (;;)
		{
			printf("\t [%d (Stack Size = %d) : ", entry->key, entry->size);

			trace_duration_t* duration = trace_hash_table_pop(table, entry->key);
			
			if (duration != NULL)
			{
				printf("%s", get_duration_name(duration));
			}
			printf("]\n");

			if (entry->next == NULL)
			{
				break;
			}

			entry = entry->next;
		}
	}
	mutex_unlock(table->lock);
}

