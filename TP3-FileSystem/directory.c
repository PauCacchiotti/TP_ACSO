#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt){
  struct inode dirinode;
  if (inode_iget(fs, dirinumber, &dirinode) < 0) return -1;

  if (!(dirinode.i_mode & IALLOC) || (dirinode.i_mode & IFMT) != IFDIR) return -1;

  int size = inode_getsize(&dirinode);
  int numBlocks = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;

  for (int bno = 0; bno < numBlocks; bno++) {
    char buf[DISKIMG_SECTOR_SIZE];
    int bytes = file_getblock(fs, dirinumber, bno, buf);
    if (bytes < 0) return -1;

    int entries = bytes / sizeof(struct direntv6);
    struct direntv6 *entriesBuf = (struct direntv6 *)buf;

    for (int i = 0; i < entries; i++) {
      if (strncmp(name, entriesBuf[i].d_name, 14) == 0) {
        *dirEnt = entriesBuf[i]; 
        return 0;
      }
    }
  }
  return -1;
}
