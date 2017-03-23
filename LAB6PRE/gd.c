/********* gd.c code ***************/
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
struct gd things!
bg_block_bitmap
bg_inode_bitmap
bg_inode_table
bg_free_blocks_count
bg_free_inodes_count
bg_used_dirs_count
bg_pad
bg_reserved
**********************************************************************/

char buf[BLKSIZE];
int fd;

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

gd()
{
  // read SUPER block
  get_block(fd, 1, buf);  
  sp = (SUPER *)buf;

  // check for EXT2 magic number:

  printf("s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53){
    printf("NOT an EXT2 FS\n");
    exit(1);
  }

  printf("EXT2 FS OK\n");


  // read Group Descriptor 0 block
  get_block(fd, 2, buf);
  gp = (SUPER *)buf;

  printf("bg_block_bitmap      = %d\n", gp->bg_block_bitmap);
  
  printf("bg_inode_bitmap      = %d\n", gp->bg_inode_bitmap);
  printf("bg_inode_table       = %d\n", gp->bg_inode_table);  
  
  printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
  printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);
  printf("bg_used_dirs_count   = %d\n", gp->bg_used_dirs_count);

  // DOESNT EXIST //printf("bg_pad               = %d\n", gp->bg_pad);
  // DOESNT EXIST //printf("bg_reserved          = %d\n", gp->bg_reserved);

  
}

char *disk = "mydisk";

main(int argc, char *argv[ ])

{ 
  if (argc > 1)
    disk = argv[1];
  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open failed\n");
    exit(1);
  }

  gd();
}

/***** SAMPLE OUTPUTs of super.c ****************
s_inodes_count                 =      184
s_blocks_count                 =     1440
s_free_inodes_count            =      174
s_free_blocks_count            =     1411
s_log_block_size               =        0
s_blocks_per_group             =     8192
s_inodes_per_group             =      184
s_mnt_count                    =        1
s_max_mnt_count                =       34
s_magic                        =     ef53
s_mtime = Mon Feb  9 07:08:22 2004
s_inode_size                   =      128
************************************************/