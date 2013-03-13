#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void check_results(char *message, int errno)
{
  if(errno < 0)
  {
    printf ("Error: %s\n", message);
    printf("Error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }
}