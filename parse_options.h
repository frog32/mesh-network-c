#ifndef __PARSE_OPTIONS_H__
#define __PARSE_OPTIONS_H__

// ********** optparse **********
// Roles that a node can play
#define HOP    -1
#define QUELLE 0
#define ZIEL   1

extern short node_role;

#define DEFAULT_TCP_PORT 3333
extern int tcp_port;

void parse_options( int argc, char *argv[]);

#endif