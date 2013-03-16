#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <sys/queue.h>
#include <pthread.h>

// connection list
SLIST_HEAD(, conn_entry) conn_head;
struct conn_entry
{
  SLIST_ENTRY(conn_entry) entries;
  unsigned int id;
  pthread_t thread;
  int fd;
  FILE *read_stream;
  FILE *write_stream;
  pthread_mutex_t write_lock;
} conn_entry;

// routing list
SLIST_HEAD(, routing_entry) routing_head;
struct routing_entry
{
  SLIST_ENTRY(routing_entry) entries;
  unsigned char target;
  struct conn_entry *conn;
} routing_entry;


struct packet_entry
{
	unsigned short id;
	struct conn_entry *source;
  unsigned char target;
	int usec;
} packet_tracker[100];


unsigned int conn_last_id = 0;

pthread_mutex_t alter_conn_entries = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t use_routing_table = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t use_packet_tracker = PTHREAD_MUTEX_INITIALIZER;

void *guard_connection(void *arg);

void wait_for_clients(int sock_fd);

void remove_client(struct conn_entry *conn);

void add_to_connection_pool(struct conn_entry *conn);



#endif