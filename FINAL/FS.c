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
// DEFINES                                                        //
////////////////////////////////////////////////////////////////////
#define BLOCK_SIZE        1024
#define BLKSIZE           1024
#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

//bool type declaration
typedef int bool;
#define true 1
#define false 0

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define BBITMAP           3
#define IBITMAP           4
#define INODEBLOCK        5
#define ROOT_INODE        2

// Default dir and regulsr file modes
#define DIR_MODE          0040777 
#define FILE_MODE         0100644
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define BUSY              1
#define KILLED            2

// Table sizes
#define NMINODES          50
#define NMOUNT            10
#define NPROC             10
#define NFD               10
#define NOFT              50

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

// Open File Table
typedef struct Oft {
  int   mode;
  int   refCount;
  struct Minode *inodeptr;
  long  offset;
} OFT;

// PROC structure
typedef struct Proc {
  int   uid;
  int   pid;
  int   gid;
  int   ppid;
  int   status;

  struct Minode *cwd;
  OFT   *fd[NFD];

  struct Proc *next;
  struct Proc *parent;
  struct Proc *child;
  struct Proc *sibling;
} PROC;

// In-memory inodes structure
typedef struct Minode {
  INODE   INODE;               // disk inode
  int     dev, ino;

  int     refCount;
  int     dirty;
  int     mounted;
  struct  Mount *mountptr;
  char    name[128];           // name string of file
} MINODE;

// Mount Table structure
typedef struct Mount {
  int    ninodes;
  int    nblocks;
  int    bmap;
  int    imap;
  int    iblock;
  int    dev, busy;
  struct Minode *mounted_inode;
  char   name[256];
  char   mount_name[64];
} MOUNT;

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
// Array of MINODES
MINODE minode[NMINODES];
// This is the root minode
MINODE *root;
// Array of Procs and the running proc
PROC   proc[NPROC], *running;
// Used for the tokenizer
char *name[32];


// Unknown, copied from original main
int dev;
int nblocks;
int ninodes;
int bmap;
int imap;
int iblock;



////////////////////////////////////////////////////////////////////
// FUNCTIONS                                                      //
////////////////////////////////////////////////////////////////////
// Gets a block from the file descriptor
int get_block(int fd, int blk, char buf[ ]) {
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

// load INODE at (dev,ino) into a minode[]; return mip->minode[]
MINODE *iget(int dev, int ino)
{
  int i, blk, disp;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;
  for (i=0; i < NMINODES; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
  for (i=0; i < NMINODES; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev; mip->ino = ino;  // assing to (dev, ino)
       mip->dirty = mip->mounted = mip->mountptr = 0;
       // get INODE of ino into buf[ ]      
       blk  = (ino-1)/8 + iblock;  // iblock = Inodes start block #
       disp = (ino-1) % 8;
       //printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);
       get_block(dev, blk, buf);
       ip = (INODE *)buf + disp;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
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

    // WHAT DOES THIS DO?
    root = iget(dev, 2);
    root->mountptr = (MOUNT *)malloc(sizeof(MOUNT));
    root->mountptr->ninodes = ninodes;
    root->mountptr->nblocks = nblocks;
    root->mountptr->dev = dev;
    root->mountptr->busy = 1;
    root->mountptr->mounted_inode = root;
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