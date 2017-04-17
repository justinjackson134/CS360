////////////////////////////////////////////////////////////////////
// EXT2 FILE SYSTEM                                               //
// 4/16/2017                                                      //
// JACOB SADOIAN                                                  //
// JUSTIN JACKSON                                                 //
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// INCLUDES                                                       //
////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h> // Used for strtok
#include <sys/stat.h> // Used for S_ISDIR

////////////////////////////////////////////////////////////////////
// TYPEDEFS                                                       //
////////////////////////////////////////////////////////////////////
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs


////////////////////////////////////////////////////////////////////
// DEFINES                                                        //
////////////////////////////////////////////////////////////////////
#define BLKSIZE 1024


////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES                                               //
////////////////////////////////////////////////////////////////////
int isDebug = 1; // IF 1, enable debug prints!

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

// Buffer used in getblock
char buf[BLKSIZE];
// File Descriptor int
int fd;
// Location of Inodes Beging Block
int InodesBeginBlock = 0;



////////////////////////////////////////////////////////////////////
// FUNCTIONS                                                      //
////////////////////////////////////////////////////////////////////
// Gets a block from the file descriptor
int get_block(int fd, int blk, char buf[ ]) {
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}






void mountRoot(char *disk)
{   
  // Open disk for read/write
  fd = open(disk, O_RDWR);
  // Check if the open succeds or fails
  if (fd < 0) {
    // If we failed, exit!
    printf("Open failed\n");
    exit(1);
  }
  // If we reach here, the file opened successfully
  printf("Opened '%s' successfully\n", disk);

  // read the SUPER block into the buffer
  get_block(fd, 1, buf);

  // Set the super pointer == buf
  sp = (SUPER *)buf;

  if(isDebug)
  {
    printf("\nSUPERBLOCK\n-------------------------\n - nblocks:          %d\n - ninodes:          %d\n - inodes_per_group: %d\n - # free inodes:    %d\n - # free blocks:    %d\n",
          sp->s_blocks_count, sp->s_inodes_count, sp->s_inodes_per_group, sp->s_free_inodes_count, sp->s_free_blocks_count);
  }

  // check for EXT2 magic number:
  // If we are invalid, inform user and exit
  if (sp->s_magic != 0xEF53) {
    printf("INVALID EXT2 FS: s_magic = %x\n", sp->s_magic);
    exit(1);
  } 
  // Else we are a valid file system!
  else
  {  
    printf("VALID EXT2 FS: s_magic = %x\n", sp->s_magic);

    // Read Group Descriptor block
    get_block(fd, 2, buf);
    
    // Set the Group Descriptor pointer == buf
    gp = (SUPER *)buf;
    
    // Set InodesBeginBlock
    InodesBeginBlock = gp->bg_inode_table;
    if(isDebug) printf("\nInodesBeginBlock: %d\n", InodesBeginBlock);

    /*// WHAT DOES THIS DO?
    root = iget(dev, 2);
    root->mountptr = (MOUNT *)malloc(sizeof(MOUNT));
    root->mountptr->ninodes = ninodes;
    root->mountptr->nblocks = nblocks;
    root->mountptr->dev = dev;
    root->mountptr->busy = 1;
    root->mountptr->mounted_inode = root;
    */
  }
}








////////////////////////////////////////////////////////////////////
// MAINLINE                                                       //
////////////////////////////////////////////////////////////////////

// Vars for mainline
// Name of disk to open
char *disk = "mydisk";

// Mainline handles opening of disk, then calls showblock()
main(int argc, char *argv[ ]) { 
    // If given a diskname, use it instead of mydisk - DEFAULT: "mydisk"
    if (argc > 1) {
      disk = argv[1];
    }
    // MOUNT ROOT
    mountRoot(disk);

    
}