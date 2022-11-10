#pragma once

#include "ecs.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct net_t net_t;

typedef struct heap_t heap_t;

typedef struct net_address_t
{
	uint8_t ip[4];
	uint16_t port;
} net_address_t;

typedef void(*net_configure_entity_callback_t)(ecs_t* ecs, ecs_entity_ref_t entity, int type, void* user);

net_t* net_create(heap_t* heap, ecs_t* ecs);
void net_destroy(net_t* net);

void net_update(net_t* net);

void net_connect(net_t* net, const net_address_t* address);
void net_disconnect_all(net_t* net);

void net_state_register_entity_type(net_t* net, int type, uint64_t component_mask, uint64_t replicated_component_mask, net_configure_entity_callback_t configure_callback, void* configure_callback_data);
void net_state_register_entity_instance(net_t* net, int type, ecs_entity_ref_t entity);

bool net_string_to_address(const char* str, net_address_t* address);
