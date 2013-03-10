#include <stdio.h>
#include "conn_io.h" // make sure we implement the same API
#include "packages.h"

int read_packet(FILE *stream, packet_struct *packet) {
  unsigned short packet_id;  // this is network byte order
  int n;

  n = fread(&packet_id, 2, 1, stream);
  if(n <= 0)
    return -1;
  packet->id = ntohs(packet_id);  // convert to host byte order
  n = fread(&(*packet).target, 1, 1, stream);
  if(n <= 0)
    return -1;
  n = fread(&(*packet).type, 1, 1, stream);
  if(n <= 0)
    return -1;
  n = fread((*packet).content, 1, 128, stream);
  if(n <= 0)
    return -1;
  return 0;
}

int send_packet(int socket_fd, packet_struct* packet) {
  size_t bytes_written,
	 total_bytes_written = 0;

  while( bytes_written != PACKAGE_LENGTH )  // packages have a fixed length
  {
    bytes_written = write( socket_fd,
		           packet + total_bytes_written,
			   PACKAGE_LENGTH   - total_bytes_written);
    if( bytes_written == -1 ) {
      perror("failed to write to client");
      return -1;
    }
    total_bytes_written += bytes_written;
  }
}

