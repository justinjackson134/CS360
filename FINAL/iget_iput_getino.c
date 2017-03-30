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
int get_block(int dev, int blk, char buf[ ])
int put_block(int dev, int blk, char buf[ ])
int tst_bit(), set_bit(), clr_bit(); 

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
  //(2). MINODE minode[100]; all with refCount=0
  //(3). MINODE *root = 0;
 }

//4.. Write C code for
//         MINODE *mip = iget(dev, ino)
//         int iput(MINDOE *mip)
//      int ino = getino(int *dev, char *pathname)

// load INODE at (dev,ino) into a minode[]; return mip->minode[]
MINODE *iget(int dev, int ino)
{
//(1). search minode[ ] array for an item pointed by mip with the SAME (dev,ino)
     if (found){
        mip->refCount++;  // inc user count by 1
        return mip;
     }

//(2). search minode[ ] array for an item pointed by mip whose refCount=0:
//       mip->refCount = 1;   // mark it in use
//       assign it to (dev, ino); 
//       initialize other fields: dirty=0; mounted=0; mountPtr=0

//(3). use mailman to compute

//       blk  = block containing THIS INODE
//       disp = which INODE in block
    
//       load blk into buf[ ];
//       INODE *ip point at INODE in buf[ ];

//       copy INODE into minode.INODE by
//       mip->INODE = *ip;

//(4). return mip;

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


int iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
//(1). mip->refCount--;
 
//(2). if (mip->refCount > 0) return;
//     if (!mip->dirty)       return;
 
//(3).  /* write INODE back to disk */

 printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

 //Use mip->ino to compute 

     //blk containing this INODE
     //disp of INODE in blk

     get_block(mip->dev, block, buf);

     ip = (INODE *)buf + disp;
     *ip = mip->INODE;

     put_block(mip->dev, block, buf);
} 

int search(MINODE *mip, char *name)
{
    // YOUR search function !!!
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
   return ino;
}



