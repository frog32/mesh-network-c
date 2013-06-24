// #include <unistd.h>      // getopt
#include <getopt.h>       // fixed version of getopt
#include <stdlib.h>      // exit
#include <stdio.h>

#include "parse_options.h"

short node_role = HOP;

int tcp_port = DEFAULT_TCP_PORT;


void parse_options( int argc, char *argv[])
{
  extern char *optarg;
  extern int optind, optopt;

  int optchar;

  while( ( optchar = getopt( argc, argv, "-hqzv" ) ) != -1 ) {
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
}
