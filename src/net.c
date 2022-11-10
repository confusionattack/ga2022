#include "net.h"

#include "debug.h"
#include "heap.h"
#include "mutex.h"
#include "queue.h"
#include "thread.h"
#include "timer.h"

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "WS2_32.lib")

enum
{
	k_net_mtu = 1024,
	k_timeout_ms = 5000,
	k_max_entity_types = 32,
	k_max_snapshots = 256,
	k_max_entities = 32,
};

typedef struct entity_type_t
{
	uint64_t component_mask;
	uint64_t replicated_component_mask;
	net_configure_entity_callback_t configure_callback;
	void* configure_callback_data;
	size_t replicated_size;
} entity_type_t;

typedef struct entity_data_t
{
	ecs_entity_ref_t ref;
	int type;
	int remote_sequence;
	int last_recved_sequence;
} entity_data_t;

typedef struct snapshot_t
{
	int sequence;
	int size;
	char data[k_net_mtu];
} snapshot_t;

typedef struct packet_t
{
	int size;
	char data[k_net_mtu];
} packet_t;

typedef struct packet_header_t
{
	int sequence;
	int ack_sequence;
} packet_header_t;

typedef struct entity_packet_header_t
{
	int type;
	int sequence;
} entity_packet_header_t;

typedef struct connection_t
{
	net_t* net;

	net_address_t address;

	int incoming_sequence;
	int ack_sequence;

	thread_t* send_thread;

	queue_t* send_queue;
	queue_t* recv_queue;

	uint32_t last_recv_ms;

	entity_data_t entities[k_max_entities];
} connection_t;

typedef struct net_t
{
	heap_t* heap;
	ecs_t* ecs;

	int sequence;

	SOCKET sock;
	thread_t* recv_thread;

	mutex_t* connections_mutex;
	connection_t connections[3];

	entity_type_t entity_types[k_max_entity_types];
	entity_data_t entities[k_max_entities];
	snapshot_t snapshots[k_max_snapshots];
} net_t;

static int recv_thread_func(void* user);
static connection_t* find_or_create_connection(net_t* net, const net_address_t* address);

static void timeout_old_connections(net_t* net);
static void snapshot_entities(net_t* net);
static void packet_send(connection_t* connection);
static void packet_recv(connection_t* connection);

net_t* net_create(heap_t* heap, ecs_t* ecs)
{
	net_t* net = heap_alloc(heap, sizeof(net_t), 8);
	memset(net, 0, sizeof(net_t));
	net->heap = heap;
	net->ecs = ecs;

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);

	net->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	net->connections_mutex = mutex_create();

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = 0;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(net->sock, (struct sockaddr*)&address, sizeof(address));

	int address_len = sizeof(address);
	getsockname(net->sock, (struct sockaddr*)&address, &address_len);
	debug_print(k_print_info, "Net bound port %d\n", ntohs(address.sin_port));

	net->recv_thread = thread_create(recv_thread_func, net);

	return net;
}

void net_destroy(net_t* net)
{
	net_disconnect_all(net);
	closesocket(net->sock);
	thread_destroy(net->recv_thread);
	WSACleanup();
	mutex_destroy(net->connections_mutex);
	heap_free(net->heap, net);
}

void net_update(net_t* net)
{
	timeout_old_connections(net);
	snapshot_entities(net);
	for (int i = 0; i < _countof(net->connections); ++i)
	{
		connection_t* c = &net->connections[i];
		if (c->address.port)
		{
			packet_send(c);
			packet_recv(c);
		}
	}
	net->sequence++;
}

void net_connect(net_t* net, const net_address_t* address)
{
	find_or_create_connection(net, address);
}

void net_disconnect_all(net_t* net)
{
	mutex_lock(net->connections_mutex);

	for (int i = 0; i < _countof(net->connections); ++i)
	{
		connection_t* c = &net->connections[i];
		if (c->address.port)
		{
			queue_push(c->send_queue, NULL);
			thread_destroy(c->send_thread);
			queue_destroy(c->send_queue);
			queue_destroy(c->recv_queue);
		}
	}
	memset(net->connections, 0, sizeof(net->connections));

	mutex_unlock(net->connections_mutex);
}

void net_state_register_entity_type(net_t* net, int type, uint64_t component_mask, uint64_t replicated_component_mask, net_configure_entity_callback_t configure_callback, void* configure_callback_data)
{
	if (type < _countof(net->entity_types))
	{
		net->entity_types[type].component_mask = component_mask;
		net->entity_types[type].replicated_component_mask = replicated_component_mask;
		net->entity_types[type].configure_callback = configure_callback;
		net->entity_types[type].configure_callback_data = configure_callback_data;
		net->entity_types[type].replicated_size = 0;
		for (int i = 0; i < sizeof(replicated_component_mask) * 8; ++i)
		{
			if (replicated_component_mask & (1ULL << i))
			{
				net->entity_types[type].replicated_size += ecs_get_component_type_size(net->ecs, i);
			}
		}
	}
	else
	{
		debug_print(k_print_warning, "Invalid entity type: %d\n", type);
	}
}

void net_state_register_entity_instance(net_t* net, int type, ecs_entity_ref_t entity)
{
	for (int i = 0; i < _countof(net->entities); ++i)
	{
		if (!ecs_is_entity_ref_valid(net->ecs, net->entities[i].ref, true))
		{
			net->entities[i].ref = entity;
			net->entities[i].type = type;
			return;
		}
	}
	debug_print(k_print_warning, "Out of space to register entity!\n");
}

bool net_string_to_address(const char* str, net_address_t* address)
{
	char address_str[256];
	strcpy_s(address_str, sizeof(address_str), str);

	char* port_str = NULL;

	char* colon = strchr(address_str, ':');
	if (colon)
	{
		*colon = '\0';
		port_str = ++colon;
	}

	struct addrinfo hints = { .ai_family = AF_INET };
	struct addrinfo* info = NULL;
	int result = getaddrinfo(address_str, NULL, (struct addrinfo*)&hints, &info);
	if (result == 0)
	{
		*address = (net_address_t){ 0 };
		struct addrinfo* it = info;
		if (it)
		{
			struct sockaddr_in* addr = (struct sockaddr_in*)it->ai_addr;
			address->ip[0] = addr->sin_addr.S_un.S_un_b.s_b1;
			address->ip[1] = addr->sin_addr.S_un.S_un_b.s_b2;
			address->ip[2] = addr->sin_addr.S_un.S_un_b.s_b3;
			address->ip[3] = addr->sin_addr.S_un.S_un_b.s_b4;
		}

		if (port_str)
		{
			address->port = (uint16_t)atoi(port_str);
		}
		freeaddrinfo(info);

		return true;
	}

	return false;
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

	mutex_lock(net->connections_mutex);

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
				c->incoming_sequence = -1;
				c->ack_sequence = -1;
				c->last_recv_ms = timer_ticks_to_ms(timer_get_ticks());
				c->send_queue = queue_create(net->heap, 3);
				c->recv_queue = queue_create(net->heap, 3);
				c->send_thread = thread_create(send_thread_func, c);

				result = c;
				break;
			}
		}
	}

	mutex_unlock(net->connections_mutex);

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
		connection->last_recv_ms = timer_ticks_to_ms(timer_get_ticks());

		queue_try_push(connection->recv_queue, packet);
	}

	return 0;
}

static void timeout_old_connections(net_t* net)
{
	mutex_lock(net->connections_mutex);

	uint32_t now = timer_ticks_to_ms(timer_get_ticks());
	for (int i = 0; i < _countof(net->connections); ++i)
	{
		connection_t* c = &net->connections[i];
		if (c->address.port && c->last_recv_ms + k_timeout_ms < now)
		{
			debug_print(k_print_info, "Disconnecting old connection.\n");

			queue_push(c->send_queue, NULL);
			thread_destroy(c->send_thread);
			queue_destroy(c->send_queue);
			queue_destroy(c->recv_queue);
			memset(c, 0, sizeof(*c));
		}
	}

	mutex_unlock(net->connections_mutex);
}

static void snapshot_entities(net_t* net)
{
	snapshot_t* snapshot = &net->snapshots[net->sequence % _countof(net->snapshots)];
	snapshot->sequence = net->sequence;

	char* cur = snapshot->data;
	const char* end = &snapshot->data[_countof(snapshot->data)];
	for (int i = 0; i < _countof(net->entities) && ecs_is_entity_ref_valid(net->ecs, net->entities[i].ref, true); ++i)
	{
		int type = net->entities[i].type;
		if (net->entity_types[type].replicated_size + sizeof(entity_packet_header_t) < (size_t)(end - cur))
		{
			entity_packet_header_t header =
			{
				.type = type,
				.sequence = net->entities[i].ref.sequence,
			};
			memcpy(cur, &header, sizeof(header));
			cur += sizeof(header);

			uint64_t mask = net->entity_types[type].replicated_component_mask;
			for (int c = 0; c < sizeof(mask) * 8; ++c)
			{
				if (mask & (1ULL << c))
				{
					void* component_data = ecs_entity_get_component(net->ecs, net->entities[i].ref, c, true);
					size_t component_size = ecs_get_component_type_size(net->ecs, c);
					memcpy(cur, component_data, component_size);
					cur += component_size;
				}
			}
		}
	}
	snapshot->size = (int)(cur - snapshot->data);
}

static size_t packet_add_entities(connection_t* connection, char* packet, size_t packet_capacity)
{
	net_t* net = connection->net;

	snapshot_t* cur_snapshot = &net->snapshots[net->sequence % _countof(net->snapshots)];
	const char* cur_iter = cur_snapshot->data;
	const char* cur_end = &cur_snapshot->data[cur_snapshot->size];

	snapshot_t* ack_snapshot = &net->snapshots[connection->ack_sequence % _countof(net->snapshots)];
	const char* ack_iter = ack_snapshot->data;
	const char* ack_end = &ack_snapshot->data[ack_snapshot->size];
	if (ack_snapshot->sequence != connection->ack_sequence)
	{
		ack_iter = ack_end;
	}

	char* packet_iter = packet;

	while (cur_iter < cur_end)
	{
		entity_packet_header_t cur_header;
		memcpy(&cur_header, cur_iter, sizeof(cur_header));

		memcpy(packet_iter, cur_iter, sizeof(cur_header));
		cur_iter += sizeof(cur_header);
		packet_iter += sizeof(cur_header);

		size_t ent_size = net->entity_types[cur_header.type].replicated_size;

		bool diff = true;
		if (ack_iter < ack_end)
		{
			entity_packet_header_t ack_header;
			memcpy(&ack_header, ack_iter, sizeof(ack_header));
			if (ack_header.sequence == cur_header.sequence)
			{
				diff = memcmp(cur_iter, &ack_iter[sizeof(ack_header)], ent_size) != 0;
				ack_iter += sizeof(ack_header) + ent_size;
			}
		}

		*packet_iter++ = diff;

		if (diff)
		{
			memcpy(packet_iter, cur_iter, ent_size);
			packet_iter += ent_size;
		}

		cur_iter += ent_size;
	}

	return (size_t)(packet_iter - packet);
}

static void packet_send(connection_t* connection)
{
	net_t* net = connection->net;

	packet_t* packet = heap_alloc(net->heap, sizeof(packet_t), 8);

	packet_header_t header =
	{
		.sequence = net->sequence,
		.ack_sequence = connection->incoming_sequence,
	};
	memcpy(packet->data, &header, sizeof(header));

	packet->size = sizeof(header);
	packet->size += (int)packet_add_entities(connection, &packet->data[packet->size], sizeof(packet->data) - packet->size);

	queue_push(connection->send_queue, packet);
}

static void packet_read_entities(connection_t* connection, char* packet, size_t packet_size)
{
	net_t* net = connection->net;

	const char* iter = packet;
	const char* end = packet + packet_size;
	while (iter < end)
	{
		entity_packet_header_t header;
		memcpy(&header, iter, sizeof(header));
		iter += sizeof(header);

		ecs_entity_ref_t ref = { .entity = -1 };
		for (int i = 0; i < _countof(connection->entities); ++i)
		{
			if (connection->entities[i].remote_sequence == header.sequence)
			{
				ref = connection->entities[i].ref;
				connection->entities[i].last_recved_sequence = net->sequence;
				break;
			}
		}

		if (!ecs_is_entity_ref_valid(net->ecs, ref, true))
		{
			ref = ecs_entity_add(net->ecs, net->entity_types[header.type].component_mask);
			void* configure_callback_data = net->entity_types[header.type].configure_callback_data;
			net->entity_types[header.type].configure_callback(net->ecs, ref, header.type, configure_callback_data);

			for (int i = 0; i < _countof(connection->entities); ++i)
			{
				if (!ecs_is_entity_ref_valid(net->ecs, connection->entities[i].ref, true))
				{
					connection->entities[i].ref = ref;
					connection->entities[i].remote_sequence = header.sequence;
					connection->entities[i].type = header.type;
					connection->entities[i].last_recved_sequence = net->sequence;
					break;
				}
			}
		}

		bool diff = *iter++ != 0;
		if (diff)
		{
			uint64_t mask = net->entity_types[header.type].replicated_component_mask;
			for (int i = 0; i < sizeof(mask) * 8; ++i)
			{
				if (mask & (1ULL << i))
				{
					void* component_data = ecs_entity_get_component(net->ecs, ref, i, true);
					size_t component_size = ecs_get_component_type_size(net->ecs, i);
					memcpy(component_data, iter, component_size);
					iter += component_size;
				}
			}
		}
	}

	// Remove entities that we didn't see in the snapshot!
	for (int i = 0; i < _countof(connection->entities); ++i)
	{
		if (ecs_is_entity_ref_valid(net->ecs, connection->entities[i].ref, true))
		{
			if (connection->entities[i].last_recved_sequence != net->sequence)
			{
				ecs_entity_remove(net->ecs, connection->entities[i].ref, true);
			}
		}
	}
}

static void packet_recv(connection_t* connection)
{
	net_t* net = connection->net;

	while (true)
	{
		packet_t* packet = queue_try_pop(connection->recv_queue);
		if (!packet || !packet->size)
		{
			break;
		}

		packet_header_t header;
		memcpy(&header, packet->data, sizeof(header));
		if (header.sequence <= connection->incoming_sequence)
		{
			continue;
		}

		connection->incoming_sequence = header.sequence;
		connection->ack_sequence = header.ack_sequence;

		packet_read_entities(connection, &packet->data[sizeof(header)], packet->size - sizeof(header));

		heap_free(net->heap, packet);
	}
}
