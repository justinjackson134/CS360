/********* showblock.c code ***************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE 1024

/******************* in <ext2fs/ext2_fs.h>*******************************
struct ext2_super_block {
  u32  s_inodes_count;       // total number of inodes
  u32  s_blocks_count;       // total number of blocks
  u32  s_r_blocks_count;     
  u32  s_free_blocks_count;  // current number of free blocks
  u32  s_free_inodes_count;  // current number of free inodes 
  u32  s_first_data_block;   // first data block in this group
  u32  s_log_block_size;     // 0 for 1KB block size
  u32  s_log_frag_size;
  u32  s_blocks_per_group;   // 8192 blocks per group 
  u32  s_frags_per_group;
  u32  s_inodes_per_group;    
  u32  s_mtime;
  u32  s_wtime;
  u16  s_mnt_count;          // number of times mounted 
  u16  s_max_mnt_count;      // mount limit
  u16  s_magic;              // 0xEF53
  // A FEW MORE non-essential fields
};
**********************************************************************/

// Vars for getblock()
char buf[BLKSIZE];
int fd;

// Gets a block from the file descriptor
int get_block(int fd, int blk, char buf[ ]) {
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

// Checks to make sure that the open fs is ext2
verifyext2fs() {
  // read SUPER block
  get_block(fd, 1, buf);  
  sp = (SUPER *)buf;

  // check for EXT2 magic number:
  printf("VALID EXT2 FS: s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53) {
    printf("NOT an EXT2 FS\n");
    exit(1);
  }

  //With the SuperBlock read in, you might as well print... nblocks, ninodes, ngroups, inodes_per_group, number of free inodes and blocks, etc.
  printf("SUPERBLOCK\n - nblocks:          %d\n - ninodes:          %d\n - ngroups:          %d\n - inodes_per_group: %d\n - # free inodes:    %d\n - # free blocks:    %d\n",
          sp->s_blocks_count, sp->s_inodes_count, 0, sp->s_inodes_per_group, sp->s_free_inodes_count, sp->s_free_blocks_count);
}

// Actual code for this assignment
showblock() {
  // Verify that the opened FS is ext2
  verifyext2fs();
  




}






// Vars for mainline
// Name of disk to open
char *disk = "mydisk";

// Mainline handles opening of disk
main(int argc, char *argv[ ]) { 
  if (argc > 1) {
    disk = argv[1];
  }
  fd = open(disk, O_RDONLY);
  if (fd < 0) {
    printf("open failed\n");
    exit(1);
  }
  
  showblock();
}