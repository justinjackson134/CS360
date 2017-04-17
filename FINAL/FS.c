/********* showblock.c code ***************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h> // Used for strtok
#include <sys/stat.h> // Used for S_ISDIR

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









///////////////////////////////////////////////////////////////
// Vars for getblock()
char buf[BLKSIZE];
int fd;

// Gets a block from the file descriptor
int get_block(int fd, int blk, char buf[ ]) {
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}










///////////////////////////////////////////////////////////////
// Checks to make sure that the open fs is ext2
verifyext2fs() {

  printf("!!!!!!!!!!!!!!!!!!! FILE descriptor: %d", fd);
  printf("buf: %d", buf);

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
  printf("\nSUPERBLOCK\n-------------------------\n - nblocks:          %d\n - ninodes:          %d\n - inodes_per_group: %d\n - # free inodes:    %d\n - # free blocks:    %d\n",
          sp->s_blocks_count, sp->s_inodes_count, sp->s_inodes_per_group, sp->s_free_inodes_count, sp->s_free_blocks_count);
}
















// Vars for mainline
// Name of disk to open
char *disk = "mydisk";

// Mainline handles opening of disk, then calls showblock()
main(int argc, char *argv[ ]) { 
  	// If given a diskname, use it instead of mydisk - DEFAULT: "mydisk"
  	if (argc > 1) {
    	disk = argv[1];
  	}
  	// Open disk for read only
  	fd = open(disk, O_RDONLY);
  	if (fd < 0) {
    	printf("Open failed\n");
    	exit(1);
  	}
  	printf("Opened '%s' for RDONLY\n", disk);

  	verifyext2fs();  
}