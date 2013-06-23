#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>      // dup
#include <string.h>      // memcpy
#include <stdlib.h>      // exit
#include <netinet/in.h>
#include <sys/time.h>

#include "packages.h"
#include "connection.h"
#include "check.h"
#include "conn_io.h"
#include "conn_server.h"
#include "parse_options.h"

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
  int res;
  // get lock on connection
  res = pthread_mutex_lock(&conn->write_lock);
  check_results("dispatch_packet lock mutex", res);

  res = send_packet(conn->write_stream, packet);
  check_results("dispatch_packet sending packet", res);
  // release lock
  res = pthread_mutex_unlock(&conn->write_lock);
  check_results("dispatch_packet release mutex", res);
}

void handle_ack_packet(packet_struct *packet, struct conn_entry *source_conn)
{
  int res;
  struct conn_entry *out_conn;
  struct routing_entry *re;  // used in foreach
  unsigned char target;

  res = pthread_mutex_lock(&use_packet_tracker);
  check_results("handle_ack_packet lock packet tracker mutex", res);

  if(packet_tracker[packet->id % 100].id != packet->id)
  {
    printf("paket nicht mehr in tracker\n");
    return;
  }

  // -1 zeigt an, dass hier nicht mehr geresetet werden muss
  packet_tracker[packet->id % 100].usec = -1;

  // verbindung merken, damit wir den mutex unlocken können während dem senden
  out_conn = packet_tracker[packet->id % 100].source;
  target = packet_tracker[packet->id % 100].target;

  // mutex lösen
  res = pthread_mutex_unlock(&use_packet_tracker);
  check_results("handle_ack_packet unlock packet tracker mutex", res);

  dispatch_packet(packet, out_conn);

  res = pthread_mutex_lock(&use_routing_table);
  check_results("handle_ack_packet lock routing table mutex", res);

  SLIST_FOREACH(re, &routing_head, entries)
  {
    if(target == re->target)
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
  res = pthread_mutex_unlock(&use_routing_table);
  check_results("handle_ack_packet unlock routing table mutex", res);

}

void handle_data_packet(packet_struct *packet, struct conn_entry *source_conn)
{
  int res;
  struct timeval t;
  struct packet_entry *packet_tracker_entry;
  struct routing_entry *re;  // used in foreach
  struct conn_entry *ce;  // used in foreach

  res = pthread_mutex_lock(&use_packet_tracker);
  check_results("handle_data_packet lock packet tracker mutex", res);
  if(packet->target == node_role)
  {
    // packet has arrived
    // todo: packet umschreiben
    packet->type = 'O';
    dispatch_packet(packet, source_conn);
    return;
  }

  if(packet_tracker[packet->id % 100].id == packet->id)
  {
    // packet already sent
    return;
  }
  gettimeofday(&t, NULL);
  packet_tracker_entry = &packet_tracker[packet->id % 100];
  packet_tracker_entry->id = packet->id;
  packet_tracker_entry->target = packet->target;
  packet_tracker_entry->source = source_conn;
  packet_tracker_entry->usec = t.tv_usec;

  res = pthread_mutex_unlock(&use_packet_tracker);
  check_results("handle_unlock_packet unlock packet tracker mutex", res);

  // lookup in routing tabelle
  res = pthread_mutex_lock(&use_routing_table);
  check_results("handle_data_packet lock routing mutex", res);

  // ce auf NULL stellen
  ce = NULL;
  SLIST_FOREACH(re, &routing_head, entries)
  {
    if(re->target == packet->target)
    {
      printf("fount in routing table\n");
      ce = re->conn;
      break;
    }
  }
  res = pthread_mutex_unlock(&use_routing_table);
  check_results("handle_data_packet unlock routing mutex", res);

  // falls ce kein nullpointer ist, wurde eine route gefunden
  if(ce)
  {
    dispatch_packet(packet, ce);
    return;
  }

  // weiterleiten an einen oder an alle
  printf("send packet to all neighbors\n");
  SLIST_FOREACH(ce, &conn_head, entries)
  {
    if(ce != source_conn)
    {
      dispatch_packet(packet, ce);
    }
  }
}

void *clean_packet_tracker(void *arg)
{
  int res, i, val1, val2;
  struct timeval t;
  struct routing_entry *re, *re_temp;  // used in foreach
 
  // time werte alle auf -1 stellen
  for(i=0;i<100;i++)
    packet_tracker[i].usec = -1;
  while(1){
    usleep(100000);
    res = pthread_mutex_lock(&use_packet_tracker);
    check_results("clean_packet_tracker lock packet tracker mutex", res);
    res = pthread_mutex_lock(&use_routing_table);
    check_results("clean_packet_tracker lock routing table mutex", res);
    gettimeofday(&t, NULL);
      val1 = (t.tv_usec + 100000) % 1000000;
      val2 = t.tv_usec % 1000000;
      for(i=0;i<100;i++)
      {
        // inaktiv
        if(packet_tracker[i].usec == -1)
          continue;
        if((packet_tracker[i].usec > val1 || packet_tracker[i].usec < val2) && val1 > val2)
          continue;
        if(packet_tracker[i].usec < val1 && packet_tracker[i].usec > val2)
          continue;
        printf("lost package %hu\n", packet_tracker[i].id);
        // this packet got lost remove it from routing
        SLIST_FOREACH_SAFE(re, &routing_head, entries, re_temp)
        {
          if(packet_tracker[i].target == re->target)
          {
            SLIST_REMOVE(&routing_head, re, routing_entry, entries);
            break;
          }
          
        }
        packet_tracker[i].usec = -1;

      }
    res = pthread_mutex_unlock(&use_packet_tracker);
    check_results("clean_packet_tracker unlock routing table mutex", res);    
    res = pthread_mutex_unlock(&use_routing_table);
    check_results("clean_packet_tracker unlock routing table mutex", res);
  }
    

}

void *guard_connection(void *arg)
{
  int res;
  packet_struct packet;  // used over and over again

  struct conn_entry *conn = (struct conn_entry *)arg;
  conn->read_stream = fdopen(conn->fd, "r");
  conn->write_stream = fdopen(dup(conn->fd), "w");

  // ready to write to this connection -> release lock
  pthread_mutex_unlock(&conn->write_lock);
  check_results("guard_connection unlock mutex", res);

  printf("new connection with id %d\n", conn->id);

  if( conn->read_stream == NULL || conn->write_stream == NULL)
  {
    printf("could not open fd");
    return NULL;
  }
  while(1)
  {
    if(read_packet(conn->read_stream, &packet) < 0)
    {
      // wait for other thrads to unlock
      printf("connection %d going down\n", conn->id);
      res = pthread_mutex_lock(&conn->write_lock);
      check_results("guard_connection lock mutex", res);
      remove_from_connection_pool(conn);
      fclose(conn->read_stream);
      fclose(conn->write_stream);
      printf("connection %d closed\n", conn->id);
      pthread_mutex_destroy(&conn->write_lock);
      free(conn);
      return NULL;
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
  pthread_attr_t pthread_custom_attr;
  pthread_t thread;

  // start thread for packet_tracker
  pthread_attr_init(&pthread_custom_attr);
  pthread_create(&thread, &pthread_custom_attr, clean_packet_tracker, NULL);

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
      printf("failed to get a client connection");
    }
    
  }

}
