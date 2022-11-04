#pragma once

// Entity Component System
// Framework for game entities and their components.

#include <stdbool.h>
#include <stdint.h>

typedef struct heap_t heap_t;

// Handle to an entity component system interface.
typedef struct ecs_t ecs_t;

// Weak reference to an entity.
typedef struct ecs_entity_ref_t
{
	int entity;
	int sequence;
} ecs_entity_ref_t;

// Working data for an active entity query.
typedef struct ecs_query_t
{
	uint64_t component_mask;
	int entity;
} ecs_query_t;

// Create an entity component system.
ecs_t* ecs_create(heap_t* heap);

// Destroy an entity component system.
void ecs_destroy(ecs_t* ecs);

// Per-frame entity component system update.
void ecs_update(ecs_t* ecs);

// Register a type of component with the entity system.
int ecs_register_component_type(ecs_t* ecs, const char* name, size_t size_per_component, size_t alignment);

// Spawn an entity with the masked components and return a reference to it.
ecs_entity_ref_t ecs_entity_add(ecs_t* ecs, uint64_t component_mask);

// Destroy an entity.
// If allow_pending_add is true, can destroy an entity that is not fully spawned.
void ecs_entity_remove(ecs_t* ecs, ecs_entity_ref_t ref, bool allow_pending_add);

// Determines if a entity reference points to a valid entity.
// If allow_pending_add is true, entities that are not fully spawned are considered valid.
bool ecs_is_entity_ref_valid(ecs_t* ecs, ecs_entity_ref_t ref, bool allow_pending_add);

// Get the memory for a component on an entity.
// NULL is returned if the entity is not valid or the component_type is not present on the entity.
// If allow_pending_add is true, will return component data for not fully spawned entities.
void* ecs_entity_get_component(ecs_t* ecs, ecs_entity_ref_t ref, int component_type, bool allow_pending_add);

// Creates a new entity query by component type mask.
ecs_query_t ecs_query_create(ecs_t* ecs, uint64_t mask);

// Determines if the query points at a valid entity.
bool ecs_query_is_valid(ecs_t* ecs, ecs_query_t* query);

// Advances the query to the next matching entity, if any.
void ecs_query_next(ecs_t* ecs, ecs_query_t* query);

// Get data for a component on the entity referenced by the query, if any.
void* ecs_query_get_component(ecs_t* ecs, ecs_query_t* query, int component_type);

// Get a entity reference for the current query location.
ecs_entity_ref_t ecs_query_get_entity(ecs_t* ecs, ecs_query_t* query);
