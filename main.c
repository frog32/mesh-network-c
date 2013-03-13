#include <stdlib.h>      // exit
#include <stdio.h>
#include <unistd.h>      // getopt

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


