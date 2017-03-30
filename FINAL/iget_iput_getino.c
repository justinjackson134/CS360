//Includes and Global Variables:

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

int dev;
int nblocks;
int ninodes;
int bmap;
int imap;
int iblock;

/********** Functions as BEFORE ***********/
int get_block(int dev, int blk, char buf[ ]){
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}
int put_block(int dev, int blk, char buf[ ]) {
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}
int tst_bit(char *buf, int bit) {
  int i, j;
  i = bit / 8;  j = bit % 8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}
int set_bit(char *buf, int bit) {
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}
int clr_bit(char *buf, int bit) {
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

/*
3. FS Level-1 Data Structures

PROC* running           MINODE *root                          
      |                          |              
      |                          |               ||*********************
      V                          |  MINODE       || 
    PROC[0]                      V minode[100]   ||         Disk dev
 =============  |-pointerToCWD-> ============    ||   ==================
 |nextProcPtr|  |                |  INODE   |    ||   |     INODEs   
 |pid = 1    |  |                | -------  |    ||   ================== 
 |uid = 0    |  |                | (dev,2)  |    || 
 |cwd --------->|                | refCount |    ||*********************
 |           |                   | dirty    |
 |fd[16]     |                   |          |         
 | ------    |       
 | - ALL 0 - |                   |==========|         
 | ------    |                   |  INODE   |          
 | ------    |                   | -------  |   
 =============                   | (dev,ino)|   
                                 |  refCount|  
   PROC[1]          ^            |  dirty   |   
    pid=2           |
    uid=1           |  
    cwd ----> root minode        |==========|  
*/

init() // Initialize data structures of LEVEL-1:
 {
  //(1). 2 PROCs, P0 with uid=0, P1 with uid=1, all PROC.cwd = 0
  proc[0] = malloc(sizeof(PROC));
  proc[0]->uid = 0;
  proc[0]->pid = 0;
  proc[0]->cwd = 0;

  proc[1] = malloc(sizeof(PROC));
  proc[1]->uid = 0;
  proc[1]->pid = 0;
  proc[1]->cwd = 0;

  //(2). MINODE minode[100]; all with refCount=0
  int i = 0;
  for (i = 0; i < 100; i++) {
    minode[i] = malloc(sizeof(MINODE));
    minode[i]->refCount = 0;
  }

  //(3). MINODE *root = 0;
  root = 0;
 }

//4.1 Write C code for
//         MINODE *mip = iget(dev, ino)
// load INODE at (dev,ino) into a minode[]; return mip->minode[]
MINODE *iget(int dev, int ino)
{
  int i, blk, disp;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;
  for (i=0; i < NMINODE; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
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

//4.2 Write C code for
//         int iput(MINDOE *mip)
int iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
  //(1). mip->refCount--;
  mip->refCount--;
 
  //(2). if (mip->refCount > 0) return;
  if(mip->refCount > 0)
    return;
  //     if (!mip->dirty)       return;
  if(!mip->dirty)
    return;
 
  //(3).  /* write INODE back to disk */
  printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

  //Use mip->ino to compute;
  //blk containing this INODE <--------------------------------------------------------------------------------------------DO THIS?
  //disp of INODE in blk

  get_block(mip->dev, block, buf);

  ip = (INODE *)buf + disp;
  *ip = mip->INODE;

  put_block(mip->dev, block, buf);
} 

// Vars for search
char dbuf[1024];
// Searches through data blocks to find entry specified by pathname
int search(MINODE *mip, char *name) {
  printf("\nSEARCHING FOR: %s", name);
  for (int i = 0; i < 12; i++) {
    if (mip->i_block[i] == 0)
      return 0;
    get_block(fd, mip->i_block[i], dbuf);  // char dbuf[1024]

    DIR *dp = (SUPER *)dbuf;
    char *cp = dbuf;

    while (cp < &dbuf[1024])
    {
      //use dp-> to print the DIR entries as  [inode rec_len name_len name]
      //printf("\n - DIR ENTRY - rec_len: %d, name_len: %d, name: %s", dp->rec_len, dp->name_len, dp->name);
      if (strcmp(name, dp->name) == 0)
      {
        //printf("\n - Name: %s == %s", name, dp->name);
        printf("\n - Found at INODE: %d\n", dp->inode);
        return dp->inode;
      }
      //printf("\n - Name: %s != %s", name, dp->name);
      cp += dp->rec_len;
      dp = (DIR *)cp;

      //getchar();
    }
    printf(" - Not Found\n");
    return 0;
  }
}

//4.3 Write C code for
//      int ino = getino(int *dev, char *pathname)
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
   return ino;
}



