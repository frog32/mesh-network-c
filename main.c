#include <stdlib.h>      // exit
#include <stdio.h>
#include <unistd.h>      // getopt

#include "conn_server.h"
#include "packages.h"
#include "check.h"
#include "parse_options.h"
#include "util.h"


int main(int argc, char *argv[])
{
  int sock_fd;
  // pthread_t pthread;

  parse_options( argc, argv );

  sock_fd = listen_on_port(tcp_port);
  check_results("failed to listen to port", sock_fd);

  dbg("initialized as role %d", node_role);

  wait_for_clients(sock_fd);

  return 0; 
}
