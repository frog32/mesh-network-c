#ifndef __PACKAGES_H__
#define __PACKAGES_H__

#define PACKAGE_LENGTH 132

typedef struct
{
  unsigned short id;
  unsigned char target;
  char type;
  char content[128];
} packet_struct;

#endif