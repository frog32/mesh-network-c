#include <stdlib.h>      // exit
#include <stdio.h>
#include <string.h>      // strlen
#include <unistd.h>      // getopt
#include <pthread.h>
#include "conn_server.h"
#include "conn_io.h"     // send_all
#include "packages.h"

void report_error( char* message ) {
  fprintf( stderr, "ERROR: %s\n", message );
}



// ********** optparse **********
// Roles that a node can play
#define HOP    0
#define ZIEL   1
#define QUELLE 2
short node_role = HOP;

#define NO_TCP_PORT -1
int   tcp_port  = NO_TCP_PORT;

void parse_options( int argc, char *argv[])
{
  extern char *optarg;
  extern int optind, optopt;

  char optchar;

  while( ( optchar = getopt( argc, argv, "-hqz" ) ) != -1 ) {
    switch( optchar ) {
      case 'z':
        node_role = ZIEL;
        break;;
      case 'q':
        node_role = QUELLE;
        break;;
      case 1: // optchar '-' will assign non-option to 1
        tcp_port = atoi(optarg);
        break;;
      case 'h':
      case '?':
      default:
        printf("Usage: server port [-z|-q]\n");
        exit( 0 );
    }
  }
  for ( ; optind < argc; optind++)
  {
    if(argc - optind == 1)
    {
      // read in port
      tcp_port = atoi(argv[optind]);
    }
  } 
  
  if (tcp_port == NO_TCP_PORT ) {
    report_error("no port provided");
    exit( -1);
  }
}

// ********** guard_connection **********

typedef struct
{
  int id;
  pthread_t thread;
  int fd;
  FILE *stream;
} conn_struct;

void *guard_connection(void *arg)
{
  packet_struct packet;  // used over and over again
  conn_struct *connection = (conn_struct *)arg;
  connection->stream = fdopen(connection->fd, "r");
  if( connection->stream == NULL )
  {
    report_error("could not open fd");
    return;
  }
  while(1)
  {
      if(read_packet(connection->stream, &packet) < 0)
      {
        // connection lost todo: handle loss
        exit(-1);
      }
      switch(packet.type){
        case 'N':
          printf("neue verbindung erstellen\n");
          break;
        case 'O':
          printf("Bestaetigung erhalten\n");
          break;
        case 'C':
          printf("Datenpaket erhalten\n");
          break;
      }
      printf("packet_id=%hu\n", packet.id);
      printf("packet_type%c\n", packet.type);
  }
}

void wait_for_clients(sock_fd)
{
  int i = 0; // doto remove
  conn_struct *connections;
  pthread_attr_t pthread_custom_attr;

  connections=(conn_struct *)malloc(5*sizeof(*connections));
  pthread_attr_init(&pthread_custom_attr);
  while(1)
  {
    if( (connections[i].fd = connect_with_client( sock_fd )) != 0) {
      pthread_create(&connections[i].thread, &pthread_custom_attr, guard_connection, (void *)(connections+i));
      i++;
    }
    else {
      report_error("failed to get a client connection");
    }
    
  }

}

int main(int argc, char *argv[])
{
  int sock_fd;
  // pthread_t pthread;

  parse_options( argc, argv );

  printf("port %d\n", tcp_port);
  printf("role %d\n", node_role);

  if( (sock_fd = listen_on_port(tcp_port) ) < 0 ) {
    report_error("failed to listen on the port");
    return sock_fd;
  }

  wait_for_clients(sock_fd);

  return 0; 
}


