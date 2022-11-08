#include "ecs.h"

#include "debug.h"
#include "heap.h"

#include <string.h>

enum
{
	k_max_component_types = 64,
	k_max_entities = 512,
};

typedef enum entity_state_t
{
	k_entity_unused,
	k_entity_pending_add,
	k_entity_active,
	k_entity_pending_remove,
} entity_state_t;

typedef struct ecs_t
{
	heap_t* heap;
	int global_sequence;

	int sequences[k_max_entities];
	entity_state_t entity_states[k_max_entities];
	uint64_t component_masks[k_max_entities];

	void* components[k_max_component_types];
	size_t component_type_sizes[k_max_component_types];
	char component_type_names[k_max_component_types][32];
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
	for (int i = 0; i < _countof(ecs->entity_states); ++i)
	{
		ecs->entity_states[i] = k_entity_unused;
	}
	for (int i = 0; i < _countof(ecs->component_masks); ++i)
	{
		ecs->component_masks[i] = 0;
	}
	return ecs;
}

void ecs_destroy(ecs_t* ecs)
{
	for (int i = 0; i < _countof(ecs->components); ++i)
	{
		if (ecs->components[i])
		{
			heap_free(ecs->heap, ecs->components[i]);
		}
	}
	heap_free(ecs->heap, ecs);
}

void ecs_update(ecs_t* ecs)
{
	for (int i = 0; i < _countof(ecs->entity_states); ++i)
	{
		if (ecs->entity_states[i] == k_entity_pending_add)
		{
			ecs->entity_states[i] = k_entity_active;
		}
		else if (ecs->entity_states[i] == k_entity_pending_remove)
		{
			ecs->entity_states[i] = k_entity_unused;
		}
	}
}

int ecs_register_component_type(ecs_t* ecs, const char* name, size_t size_per_component, size_t alignment)
{
	for (int i = 0; i < _countof(ecs->components); ++i)
	{
		if (ecs->components[i] == NULL)
		{
			size_t aligned_size = (size_per_component + (alignment - 1)) & ~(alignment - 1);
			strcpy_s(ecs->component_type_names[i], sizeof(ecs->component_type_names[i]), name);
			ecs->component_type_sizes[i] = aligned_size;
			ecs->components[i] = heap_alloc(ecs->heap, aligned_size * k_max_entities, alignment);
			memset(ecs->components[i], 0, aligned_size * k_max_entities);
			return i;
		}
	}
	debug_print(k_print_warning, "Out of component types.");
	return -1;
}

size_t ecs_get_component_type_size(ecs_t* ecs, int component_type)
{
	return ecs->component_type_sizes[component_type];
}

ecs_entity_ref_t ecs_entity_add(ecs_t* ecs, uint64_t component_mask)
{
	for (int i = 0; i < _countof(ecs->entity_states); ++i)
	{
		if (ecs->entity_states[i] == k_entity_unused)
		{
			ecs->entity_states[i] = k_entity_pending_add;
			ecs->sequences[i] = ecs->global_sequence++;
			ecs->component_masks[i] = component_mask;
			return (ecs_entity_ref_t) { .entity = i, .sequence = ecs->sequences[i] };
		}
	}
	debug_print(k_print_warning, "Out of entities.");
	return (ecs_entity_ref_t) { .entity = -1, .sequence = -1 };
}

void ecs_entity_remove(ecs_t* ecs, ecs_entity_ref_t ref, bool allow_pending_add)
{
	if (ecs_is_entity_ref_valid(ecs, ref, allow_pending_add))
	{
		ecs->entity_states[ref.entity] = k_entity_pending_remove;
	}
	else
	{
		debug_print(k_print_warning, "Attempting to remove inactive entity.");
	}
}

bool ecs_is_entity_ref_valid(ecs_t* ecs, ecs_entity_ref_t ref, bool allow_pending_add)
{
	return ref.entity >= 0 &&
		ecs->sequences[ref.entity] == ref.sequence &&
		ecs->entity_states[ref.entity] >= (allow_pending_add ? k_entity_pending_add : k_entity_active);
}

void* ecs_entity_get_component(ecs_t* ecs, ecs_entity_ref_t ref, int component_type, bool allow_pending_add)
{
	if (ecs_is_entity_ref_valid(ecs, ref, allow_pending_add) && ecs->components[component_type])
	{
		char* components = ecs->components[component_type];
		return &components[ecs->component_type_sizes[component_type] * ref.entity];
	}
	return NULL;
}

ecs_query_t ecs_query_create(ecs_t* ecs, uint64_t mask)
{
	ecs_query_t query = { .component_mask = mask, .entity = -1 };
	ecs_query_next(ecs, &query);
	return query;
}

bool ecs_query_is_valid(ecs_t* ecs, ecs_query_t* query)
{
	return query->entity >= 0;
}

void ecs_query_next(ecs_t* ecs, ecs_query_t* query)
{
	for (int i = query->entity + 1; i < _countof(ecs->component_masks); ++i)
	{
		if ((ecs->component_masks[i] & query->component_mask) == query->component_mask && ecs->entity_states[i] >= k_entity_active)
		{
			query->entity = i;
			return;
		}
	}
	query->entity = -1;
}

void* ecs_query_get_component(ecs_t* ecs, ecs_query_t* query, int component_type)
{
	char* components = ecs->components[component_type];
	return &components[ecs->component_type_sizes[component_type] * query->entity];
}

ecs_entity_ref_t ecs_query_get_entity(ecs_t* ecs, ecs_query_t* query)
{
	return (ecs_entity_ref_t) { .entity = query->entity, .sequence = ecs->sequences[query->entity] };
}
