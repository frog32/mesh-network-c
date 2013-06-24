#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void check_results(char *message, int errno)
{
  if(errno < 0)
  {
    fprintf (stderr, "Error: %s\n", message);
    fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }
}