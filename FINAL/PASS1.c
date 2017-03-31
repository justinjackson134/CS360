
/*                   PROJECT Assignment #1
  
              MUST complete this work THIS WEEK
   
        Wed:    3/29: work on THIS code in class
        Friday: 3/31: Bring in your LABTOP to show you can mount root and ls,cd

                      QUIZ on Monday 4/3/2017


OBJECTIVE: mount root to start the PROJECT; ls, cd, pwd to show FS contents
*/

//GIVEN BASE Code: Complete THIS code during class lecture.

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h> 

//#include "type.h" // ALREADY INCLUDED IN IGET

#include "iget_iput_getino.c"  // YOUR iget_iput_getino.c file with
                               // get_block/put_block, tst/set/clr bit functions

// global variablesw
MINODE minode[NMINODE];        // global minode[ ] array           
MINODE *root;                  // root pointer: the /    
PROC   proc[NPROC], *running;  // PROC; using only proc[0]

int fd, dev;                               // file descriptor or dev
int nblocks, ninodes, bmap, imap, iblock;  // FS constants


char *disk = "mydisk";
char line[128], cmd[64], pathname[64];
char buf[BLKSIZE];              // define buf1[ ], buf2[ ], etc. as you need



///////////////////////////////////////////////////////////////
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
  printf("\nSUPERBLOCK\n-------------------------\n - nblocks:          %d\n - ninodes:          %d\n - inodes_per_group: %d\n - # free inodes:    %d\n - # free blocks:    %d\n",
          sp->s_blocks_count, sp->s_inodes_count, sp->s_inodes_per_group, sp->s_free_inodes_count, sp->s_free_blocks_count);
}



main(int argc, char *argv[ ])   // run as a.out [diskname]
{
  if (argc > 1)
     disk = argv[1];

  if ((dev = fd = open(disk, O_RDWR)) < 0){
     printf("open %s failed\n", disk);  
     exit(1);
  }
  //print fd or dev to see its value!!!
  printf("dev = %d\n", dev);

  //(1). printf("checking EXT2 FS\n");
  verifyext2fs();
     //Write C code to check EXT2 FS; if not EXT2 FS: exit
     //get ninodes, nblocks (and print their values)


  //(2). Read GD block to get bmap, imap, iblock (and print their values)

  //(3). init();         // write C code 

  //(4). mount_root();   // write C code

  //(5). printf("creating P0 as running process\n");

    //WRITE C code to do these:     
      //set running pointer to point at proc[0];
      //set running's cwd   to point at / in memory;


  //(6). while(1){       // command processing loop
  //printf("input command : [ls|cd|pwd|quit] ");

  //WRITE C code to do these:

  //use fgets() to get user inputs into line[128]
  //kill the \r at end 
  //if (line is an empty string) // if user entered only \r
      //continue;

  //Use sscanf() to extract cmd[ ] and pathname[] from line[128]
  //printf("cmd=%s pathname=%s\n", cmd, pathname);

  // execute the cmd
  if (strcmp(cmd, "ls")==0)
      ls(pathname);
  if (strcmp(cmd, "cd")==0)
      chdir(pathname);
  if (strcmp(cmd, "pwd")==0)
      pwd(running->cwd);
  if (strcmp(cmd, "quit")==0)
      quit();
}


int init()
{
  /* WRITE C code to initialize

   (1).All minode's refCount=0;
   (2).proc[0]'s pid=1, uid=0, cwd=0, fd[ ]=0;
       proc[1]'s pid=2, uid=1, cwd=0, fd[ ]=0;*/
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);         // Do you understand this?
}
 
int ls(char *pathname)  // dig out YOUR OLD lab work for ls() code 
{
  /*WRITE C code for these:

    determine initial dev: 
    if pathname[0]== '/': dev = root->dev;
    else                : dev = running->cwd->dev;

    convert pathname to (dev, ino);
    get a MINODE *mip pointing at a minode[ ] with (dev, ino);
 
    
    if mip->INODE is a file: ls_file(pathname);
    if mip->INODE is a DIR{
       step through DIR entries:
       for each name, ls_file(pathname/name);
    }*/
}

int chdir(pathname)
{
  /*Write C code to do these:

    determine initial dev as in ls()
    convert pathname to (dev, ino);
    get a MINODE *mip pointing at (dev, ino) in a minode[ ];

    if mip->INODE is NOT a DIR: reject and print error message;

    if mip->INODE is a DIR{
       dispose of OLD cwd;
       set cwd to mip;
    }*/
}   
 
//int pwd(running->cwd): YOU WRITE CODE FOR THIS ONE!!!

int quit()
{
  /*for each minode[ ] do {
      if  minode[ ]'s refCount != 0: 
          write its INODE back to disk; 
  }*/
  exit(0);  // terminate program
}


//======================================================================
//SAMPLE solution:

//~samples/P1/p1 with mydisk