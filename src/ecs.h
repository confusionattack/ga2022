#pragma once

typedef struct heap_t heap_t;

typedef struct ecs_t ecs_t;

typedef struct ecs_entity_ref_t
{
	int entity;
	int sequence;
} ecs_entity_ref_t;

typedef struct ecs_component_t
{
	int foo;
	// state?
	// behavior?
} ecs_component_t;

ecs_t* ecs_create(heap_t* heap);
void ecs_destroy(ecs_t* ecs);

int ecs_register_component_type(ecs_t* ecs, size_t size_per_component);

int ecs_entity_add(ecs_t* ecs, ecs_component_t* components, int component_count);
void ecs_entity_remove(ecs_t* ecs, int entity);
