#include "net.h"

#include "debug.h"
#include "heap.h"
#include "queue.h"
#include "thread.h"

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "WS2_32.lib")

enum
{
	k_net_mtu = 1024,
};

typedef struct packet_t
{
	int size;
	char data[k_net_mtu];
} packet_t;

typedef struct connection_t
{
	net_t* net;

	net_address_t address;

	int outgoing_sequence;
	int incoming_sequence;
	int ack_sequence;

	thread_t* send_thread;

	queue_t* send_queue;
	queue_t* recv_queue;
} connection_t;

typedef struct net_t
{
	heap_t* heap;

	SOCKET sock;
	thread_t* recv_thread;

	connection_t connections[3];
} net_t;

static int recv_thread_func(void* user);
static connection_t* find_or_create_connection(net_t* net, const net_address_t* address);

net_t* net_create(heap_t* heap, int port)
{
	net_t* net = heap_alloc(heap, sizeof(net_t), 8);
	memset(net, 0, sizeof(net_t));
	net->heap = heap;

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);

	net->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(net->sock, (struct sockaddr*)&address, sizeof(address));

	net->recv_thread = thread_create(recv_thread_func, net);

	return net;
}

void net_destroy(net_t* net)
{
	closesocket(net->sock);
	thread_destroy(net->recv_thread);
	WSACleanup();
	heap_free(net->heap, net);
}

void net_update(net_t* net)
{
	// Send a packet to all connections!
	for (int i = 0; i < _countof(net->connections); ++i)
	{
		connection_t* c = &net->connections[i];
		if (c->address.port)
		{
			packet_t* packet = heap_alloc(net->heap, sizeof(packet_t), 8);
			strcpy_s(packet->data, sizeof(packet->data), "hello world!");
			packet->size = (int)strlen(packet->data) + 1;
			queue_push(c->send_queue, packet);
		}
	}

	// Recv from all connections!
	for (int i = 0; i < _countof(net->connections); ++i)
	{
		connection_t* c = &net->connections[i];
		if (c->address.port)
		{
			while (true)
			{
				packet_t* packet = queue_try_pop(c->recv_queue);
				if (packet)
				{
					debug_print(k_print_info, "RECVED PACKET: %s\n", packet->data);
				}
				else
				{
					break;
				}
			}
		}
	}
}

void net_connect(net_t* net, const net_address_t* address)
{
	find_or_create_connection(net, address);
}

void net_disconnect(net_t* net)
{
	// todo
}

static int send_thread_func(void* user)
{
	connection_t* connection = user;

	struct sockaddr_in address = { 0 };
	address.sin_family = AF_INET;
	address.sin_port = htons(connection->address.port);
	address.sin_addr.S_un.S_un_b.s_b1 = connection->address.ip[0];
	address.sin_addr.S_un.S_un_b.s_b2 = connection->address.ip[1];
	address.sin_addr.S_un.S_un_b.s_b3 = connection->address.ip[2];
	address.sin_addr.S_un.S_un_b.s_b4 = connection->address.ip[3];

	while (true)
	{
		packet_t* packet = queue_pop(connection->send_queue);
		if (!packet)
		{
			break;
		}

		int bytes = sendto(connection->net->sock,
			packet->data, packet->size, 0,
			(struct sockaddr*)&address, sizeof(address));

		heap_free(connection->net->heap, packet);

		debug_print(k_print_info, "sendto returned: %d\n", bytes);
		if (bytes <= 0)
		{
			break;
		}
	}

	return 0;
}

static connection_t* find_or_create_connection(net_t* net, const net_address_t* address)
{
	connection_t* result = NULL;
	for (int i = 0; i < _countof(net->connections); ++i)
	{
		connection_t* c = &net->connections[i];
		if (memcmp(&c->address, address, sizeof(net_address_t)) == 0)
		{
			result = c;
			break;
		}
	}
	if (!result)
	{
		for (int i = 0; i < _countof(net->connections); ++i)
		{
			connection_t* c = &net->connections[i];
			if (c->address.port == 0)
			{
				memcpy(&c->address, address, sizeof(*address));
				c->net = net;
				c->send_queue = queue_create(net->heap, 3);
				c->recv_queue = queue_create(net->heap, 3);
				c->send_thread = thread_create(send_thread_func, c);

				result = c;
				break;
			}
		}
	}
	return result;
}

static int recv_thread_func(void* user)
{
	net_t* net = user;

	while (true)
	{
		packet_t* packet = heap_alloc(net->heap, sizeof(packet_t), 8);

		struct sockaddr_in address;
		int address_len = sizeof(address);
		int bytes = recvfrom(net->sock,
			packet->data, sizeof(packet->data), 0,
			(struct sockaddr*)&address, &address_len);
		debug_print(k_print_info, "recvfrom returned: %d\n", bytes);
		if (bytes <= 0)
		{
			heap_free(net->heap, packet);
			break;
		}

		packet->size = bytes;

		net_address_t net_addr;
		net_addr.port = ntohs(address.sin_port);
		net_addr.ip[0] = address.sin_addr.S_un.S_un_b.s_b1;
		net_addr.ip[1] = address.sin_addr.S_un.S_un_b.s_b2;
		net_addr.ip[2] = address.sin_addr.S_un.S_un_b.s_b3;
		net_addr.ip[3] = address.sin_addr.S_un.S_un_b.s_b4;

		connection_t* connection = find_or_create_connection(net, &net_addr);
		if (!connection)
		{
			debug_print(k_print_info, "Too many connections!\n");
			heap_free(net->heap, packet);
			continue;
		}

		queue_push(connection->recv_queue, packet);
	}

	return 0;
}
