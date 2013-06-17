#include <stdio.h>
#include "conn_io.h" // make sure we implement the same API
#include <netinet/in.h>
#include "packages.h"

int read_packet(FILE *stream, packet_struct *packet) {
  unsigned short packet_id;  // this is network byte order
  int n;

  n = fread(&packet_id, 2, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  packet->id = ntohs(packet_id);  // convert to host byte order
  n = fread(&packet->target, 1, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  n = fread(&packet->type, 1, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  n = fread(&packet->content, 128, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  return 0;
}

int send_packet(FILE *stream, packet_struct *packet) {
  unsigned short packet_id;  // this is network byte order
  int n;
  printf("send_packet %hu\n", packet->id);
  packet_id = htons(packet->id);
  n = fwrite(&packet_id, 2, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  n = fwrite(&packet->target, 1, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  n = fwrite(&packet->type, 1, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  n = fwrite(&packet->content, 128, 1, stream);
  if(n < 0)
    return n;
  if(!n)
    return -1;
  fflush(stream);
  return 0;
}

