#include <sys/socket.h>
#include <stdio.h>
#include <string.h>      // memcpy
#include <stdlib.h>      // exit
#include <netinet/in.h>

#include "packages.h"
#include "connection.h"

int create_outgoing_connection(packet_struct *packet)
{
  struct conn_entry *conn;
  struct sockaddr_in serv_addr;

  conn = malloc(sizeof(*conn));

  if( (conn->fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0 ){
    perror( "ERROR opening socket" );
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  memcpy(&(serv_addr.sin_port), &(packet->content[4]), 2);
  memcpy(&(serv_addr.sin_addr), &(packet->content), 4);

  if( connect( conn->fd, (struct sockaddr *)&serv_addr, sizeof( serv_addr )) == -1 ){
    perror( "ERROR connecting" );
    return -1;
  }
  add_to_connection_pool(conn);
  return 0;
}

void add_to_connection_pool(struct conn_entry *conn)
{
  pthread_attr_t pthread_custom_attr;

  // get lock
  pthread_mutex_lock(&alter_conn_entries);

  // init write lock for this connection and lock it
  pthread_mutex_init(&conn->write_lock, NULL);
  pthread_mutex_lock(&conn->write_lock);

  conn->id = conn_last_id++;
  // insert new connection into list
  SLIST_INSERT_HEAD(&conn_head, conn, entries);

  // start new thread
  pthread_attr_init(&pthread_custom_attr);
  pthread_create(&conn->thread, &pthread_custom_attr, guard_connection, (void *)(conn));


  // release lock
  pthread_mutex_unlock(&alter_conn_entries);
}

void remove_from_connection_pool(struct conn_entry *conn)
{
  struct routing_entry *re, *re_temp;  // used in foreach

  // get locks
  pthread_mutex_lock(&alter_conn_entries);
  pthread_mutex_lock(&use_routing_table);


  SLIST_REMOVE(&conn_head, conn, conn_entry, entries);

  // remove all routes
  SLIST_FOREACH_SAFE(re, &routing_head, entries, re_temp) {
    if(re->conn == conn)
    {
      printf("remove obsolete route to %d\n", re->target);
      SLIST_REMOVE(&routing_head, re, routing_entry, entries);
      free(re);
    }
  }
  // release lock
  pthread_mutex_unlock(&alter_conn_entries);
  pthread_mutex_unlock(&use_routing_table);
}

void dispatch_packet(packet_struct *packet, struct conn_entry *conn)
{
  // get lock on connection
  pthread_mutex_lock(&conn->write_lock);
  printf("dispatch packet %hu on connection %i\n", packet->id, conn->id);
  send_packet(conn->write_stream, packet);

  // release lock
  pthread_mutex_unlock(&conn->write_lock);
}

void handle_ack_packet(packet_struct *packet, struct conn_entry *source_conn)
{
  struct routing_entry *re;  // used in foreach

  if(packet_tracker[packet->id % 100].id != packet->id)
  {
    printf("paket nicht mehr in tracker\n");
    return;
  }
  // packet_tracker[packet->id % 100].timestamp = NULL;
  dispatch_packet(packet, packet_tracker[packet->id % 100].source);

  pthread_mutex_lock(&use_routing_table);
  SLIST_FOREACH(re, &routing_head, entries)
  {
    if(packet_tracker[packet->id % 100].target == re->target)
    {
      // overwrite connection
      re->conn = source_conn;
      break;
    }
    
  }

  if(!re || re->conn != source_conn)
  {
    re = malloc(sizeof(struct routing_entry));
    re->conn = source_conn;
    re->target = packet_tracker[packet->id % 100].target;
    SLIST_INSERT_HEAD(&routing_head, re, entries);
    printf("enter packet into routes\n");
  }
  pthread_mutex_unlock(&use_routing_table);
}

void handle_data_packet(packet_struct *packet, struct conn_entry *source_conn)
{
  struct packet_entry *packet_tracker_entry;
  struct routing_entry *re;  // used in foreach
  struct conn_entry *ce;  // used in foreach
  if(packet_tracker[packet->id % 100].id == packet->id)
  {
    printf("paket %hu in tracker\n", packet->id);
    // packet already sent
    return;
  }
  printf("paket %hu nicht in tracker\n", packet->id);
  packet_tracker_entry = &packet_tracker[packet->id % 100];
  packet_tracker_entry->id = packet->id;
  packet_tracker_entry->target = packet->target;
  packet_tracker_entry->source = source_conn;

  // lookup in routing tabelle
  pthread_mutex_lock(&use_routing_table);

  SLIST_FOREACH(re, &routing_head, entries)
  {
    if(re->target == packet->target)
    {
      printf("fount in routing table\n");
      dispatch_packet(packet, re->conn);
      return;
    }
  }
  pthread_mutex_unlock(&use_routing_table);
  
  // weiterleiten an einen oder an alle
  printf("send packet to all neighbors\n");
  SLIST_FOREACH(ce, &conn_head, entries)
  {
    printf("send packet to %hu\n", ce->id);
    if(ce != source_conn)
    {
      dispatch_packet(packet, ce);
    }
  }
}

void *guard_connection(void *arg)
{
  packet_struct packet;  // used over and over again

  struct conn_entry *conn = (struct conn_entry *)arg;
  conn->read_stream = fdopen(conn->fd, "r");
  conn->write_stream = fdopen(dup(conn->fd), "w");

  // ready to write to this connection -> release lock
  pthread_mutex_unlock(&conn->write_lock);
  printf("new connection with id %d\n", conn->id);

  if( conn->read_stream == NULL || conn->write_stream == NULL)
  {
    report_error("could not open fd");
    return;
  }
  while(1)
  {
      if(read_packet(conn->read_stream, &packet) < 0)
      {
        // wait for other thrads to unlock
        pthread_mutex_lock(&conn->write_lock);
        remove_from_connection_pool(conn);
        fclose(conn->read_stream);
        fclose(conn->write_stream);
        printf("connection %d closed\n", conn->id);
        pthread_mutex_destroy(&conn->write_lock);
        free(conn);
        return;
      }
      switch(packet.type){
        case 'N':
          printf("neue Verbindung erstellen\n");
          create_outgoing_connection(&packet);
          break;
        case 'O':
          printf("Bestaetigung erhalten\n");
          handle_ack_packet(&packet, conn);
          break;
        case 'C':
          printf("Datenpaket erhalten\n");
          handle_data_packet(&packet, conn);
          break;
      }

  }
}

void wait_for_clients(int sock_fd)
{
  struct conn_entry *conn;

  conn = malloc(sizeof(*conn));

  // init connections list
  SLIST_INIT(&conn_head);

  // init routing table
  SLIST_INIT(&routing_head);

  while(1)
  {
    if( (conn->fd = connect_with_client( sock_fd )) != 0) {
      // add to connection list
      add_to_connection_pool(conn);
      // get new memory for next connection
      conn = malloc(sizeof(*conn));
    }
    else {
      report_error("failed to get a client connection");
    }
    
  }

}
