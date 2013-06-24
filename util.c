#include <stdio.h>
#include <stdarg.h>
#include "parse_options.h"

void dbg(char *msg_fmt, ...){
  va_list arg;
  char message[128];

  va_start(arg, msg_fmt);
  vsprintf(message, msg_fmt, arg);
  va_end (arg);
  fprintf(stderr, "%d %s\n", tcp_port, message);
}