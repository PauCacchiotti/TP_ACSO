#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1 || inumber >= fs->superblock.s_isize * 16) return -1;

    int sector = INODE_START_SECTOR + (inumber - 1) / 16;

    struct inode inodes[16];
    int bytes = diskimg_readsector(fs->dfd, sector, inodes);
    if (bytes != DISKIMG_SECTOR_SIZE) return -1;  

    int offset = (inumber - 1) % 16;
    *inp = inodes[offset];

    return 0;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if (!(inp->i_mode & ILARG)) {
        if (blockNum < 0 || blockNum >= 8) return -1;
        return inp->i_addr[blockNum];
    }

    if (blockNum < 7 * 256) {
        int indirectBlock = blockNum / 256;
        int offset = blockNum % 256;

        uint16_t pointers[256];
        int sector = inp->i_addr[indirectBlock];
        if (diskimg_readsector(fs->dfd, sector, pointers) != DISKIMG_SECTOR_SIZE) return -1;

        return pointers[offset];
    }

    blockNum -= 7 * 256;
    if (blockNum >= 256 * 256) return -1;

    int firstLevelIndex = blockNum / 256;
    int secondLevelIndex = blockNum % 256;

    uint16_t indirectPointers[256];
    int dblSector = inp->i_addr[7];
    if (diskimg_readsector(fs->dfd, dblSector, indirectPointers) != DISKIMG_SECTOR_SIZE) {
        return -1;
    }

    int indirectSector = indirectPointers[firstLevelIndex];
    if (diskimg_readsector(fs->dfd, indirectSector, indirectPointers) != DISKIMG_SECTOR_SIZE) {
        return -1;
    }

    return indirectPointers[secondLevelIndex];
}



int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
