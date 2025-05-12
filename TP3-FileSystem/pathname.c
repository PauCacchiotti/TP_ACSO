
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
  if (!pathname || pathname[0] != '/') return -1;

  int inumber = ROOT_INUMBER; 

  const char *current = pathname + 1; 
  char name[15];  

  while (*current != '\0') {
    int len = 0;
    while (*current != '/' && *current != '\0' && len < 14) {
      name[len++] = *current++;
    }
    name[len] = '\0';

    while (*current == '/') {
      current++;
    }

    struct direntv6 entry;
    if (directory_findname(fs, name, inumber, &entry) < 0) return -1;

    inumber = entry.d_inumber;  
  }

  return inumber;
}
