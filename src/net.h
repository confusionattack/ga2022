#pragma once

#include <stdint.h>

typedef struct net_t net_t;

typedef struct heap_t heap_t;

typedef struct net_address_t
{
	uint8_t ip[4];
	uint16_t port;
} net_address_t;

net_t* net_create(heap_t* heap, int port);
void net_destroy(net_t* net);

void net_update(net_t* net);

void net_listen(net_t* net);
void net_connect(net_t* net, const net_address_t* address);
void net_disconnect(net_t* net);
