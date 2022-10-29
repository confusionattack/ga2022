#include "ecs.h"

#include "heap.h"

#include <stdint.h>
#include <string.h>

enum
{
	k_max_component_types = 64,
	k_max_entities = 512,
};

typedef struct ecs_t
{
	heap_t* heap;
	int global_sequence;
	int sequences[k_max_entities];
	void* components[k_max_component_types];
} ecs_t;

ecs_t* ecs_create(heap_t* heap)
{
	ecs_t* ecs = heap_alloc(heap, sizeof(ecs_t), 8);
	ecs->heap = heap;
	ecs->global_sequence = 0;
	for (int i = 0; i < _countof(ecs->components); ++i)
	{
		ecs->components[i] = NULL;
	}
	for (int i = 0; i < _countof(ecs->sequences); ++i)
	{
		ecs->sequences[i] = -1;
	}
	return ecs;
}

void ecs_destroy(ecs_t* ecs)
{
	for (int i = 0; i < _countof(ecs->components); ++i)
	{
		if (ecs->components[i])
		{
			heap_free(ecs->components[i], ecs);
		}
	}
	heap_free(ecs->heap, ecs);
}

int ecs_register_component_type(ecs_t* ecs, size_t size_per_component)
{
	for (int i = 0; i < _countof(ecs->components); ++i)
	{
		if (ecs->components[i] == NULL)
		{
			ecs->components[i] = heap_alloc(ecs->heap, size_per_component * k_max_entities, 8);
			memset(ecs->components[i], 0, size_per_component * k_max_entities);
			return i;
		}
	}
	return -1;
}

int ecs_entity_add(ecs_t* ecs, ecs_component_t* components, int component_count)
{
	return -1;
}

void ecs_entity_remove(ecs_t* ecs, int entity)
{

}
