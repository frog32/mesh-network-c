#include <stdio.h>
#include <stdarg.h>
#include "parse_options.h"

void dbg(char *msg_fmt, ...){
  va_list arg;

  va_start(arg, msg_fmt);
	fprintf(stderr, "%d ", tcp_port);
	vfprintf(stderr, msg_fmt, arg);
	fprintf(stderr, "\n");

  va_end (arg);
}