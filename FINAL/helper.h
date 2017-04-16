#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define BLKSIZE     1024

#define NMINODE      100
#define NFD           16
#define NPROC          4

#define BITS_PER_BLOCK    (8*BLKSIZE)
#define INODES_PER_BLOCK  (BLKSIZE/sizeof(INODE))


typedef struct minode{
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
  int mounted;
  struct mntable *mptr;
}MINODE;

typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          uid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

typedef struct Mount {
	int  ninodes;
	int  nblocks;
	int  bmap;
	int  imap;
	int  iblock;
	int  dev, busy;
	struct Minode *mounted_inode;
	char   name[256];
	char   mount_name[64];
} MOUNT;
MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

int dev;
int nblocks;
int ninodes;
int bmap;
int imap;
int iblock;

void init()
{
 
  proc[0].uid = 0;
  proc[0].pid = 0;
  proc[0].cwd = 0;

  proc[1].uid = 1;
  proc[1].pid = 0;
  proc[1].cwd = 0;

  for(int i = 0; i < NMINODE; i++)
     minode[i].refCount = 0;

  root = 0;
  running = &proc[0];

}

void mountRoot(char *name)
{

   char buf[BLKSIZE];
   int fd = open("mydisk",O_RDWR);
   if(fd < 0)//error opening
   {
      printf("Error opening disk for reading\n\n");
      exit(1);
   }
   get_block(fd,1,buf);
   sp=(SUPER *)buf;
   if(sp->s_magic != 0xEF53)
   {
      printf("NOT EXT2 FILESYSTEM!\n\n");
      exit(1);
   }
   else
   {
	   get_block(dev, 2, buf);
	   gp = (SUPER *)buf;
	   root = iget(dev, 2);
	   root->mountptr = (MOUNT *)malloc(sizeof(MOUNT));
	   root->mountptr->ninodes = ninodes;
	   root->mountptr->nblocks = nblocks;
	   root->mountptr->dev = dev;
	   root->mountptr->busy = 1;
	   root->mountptr->mounted_inode = root;
   }
}

// read the block of data from the file device (fd) into the buffer (buf).
void get_block(int fd,int block, char *buf)
{
  lseek(fd,(long)(BLKSIZE*block),0);
  read(fd,buf, BLKSIZE);
}

// put the block of data from the buffer (buf) into the file device (fd)
void put_block(int fd, int block, char *buf)
{
  lseek(fd, (long)(BLKSIZE*block),0);
  write(fd, buf, BLKSIZE);
}

MINODE *iget(int dev, int ino)
{
  int i, blk, disp;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;
  for (i=0; i < NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
  for (i=0; i < NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev; mip->ino = ino;  // assing to (dev, ino)
       mip->dirty = mip->mounted = mip->mountPtr = 0;
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

int iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
  int nodeIn,blockIn;
  mip->refCount--;
  if (mip->refCount > 0) return;
  if (!mip->dirty)       return;
  
  printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

  nodeIn = (mip->ino -1 ) % INODES_PER_BLOCK; // Mailman's Algorithm
  blockIn = (mip->ino -1) / INODES_PER_BLOCK + INODEBLOCK; // Mailman's Algorithm

  get_block(mip->dev,blockIn, buf);
  ip = (INODE *)buf;
  ip += nodeIn;
  *ip = mip->INODE;
  put_block(mip->dev,blockIn,buf); // save the minode!
}
  

int getino(int *dev, char *pathname)
{
  int i, ino, blk, disp;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  if (pathname[0]=='/')
     mip = iget(*dev, 2);
  else
     mip = iget(running->cwd->dev, running->cwd->ino);

  strcpy(buf, pathname);
  tokenize(buf); // n = number of token strings

  for (i=0; i < n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, kcwname[i]);
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);
      mip = iget(*dev, ino);
  }
  iput(mip);
  return ino;
}
void execute_command(char *cmd)
{

}






bool search(char *filename){
//search for the given file
}


//we can add the rest as we need it
