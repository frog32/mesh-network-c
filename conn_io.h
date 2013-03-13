#ifndef __CONN_IO_H__
#define __CONN_IO_H__

#include "packages.h"

int read_packet(FILE *stream, packet_struct *packet);

/*
 * A simple wrappter around "write", that tries to send out all bytes
 * given to it. Returns -1 on error.
 */
int send_packet(FILE *stream, packet_struct* packet);
#endif
