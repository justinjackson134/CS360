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
// Used for the tokenizer //////////////////////////////////////////////I dont think this is used anywhere
char *name[32];
// Command tokenizer!
char *command[32];
// Number of commands returned by the command tokenizer
int numberOfCommands = 0;
// Used for path tokenizer
char *path[32];
// Number of path items returned by path tokenizer
int numberOfPathItems = 0;

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
       printf("\nfound [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
  for (i=0; i < NMINODES; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       printf("\nallocating NEW minode[%d] for [%d %d]\n", i, dev, ino);  // WHAT IS DEV? EARLIER WE GET GROUP DESCRIPTOR, IT USED TO SAY DEV, BUT THAT DIDNT WORK SO I MADE IT FD
       // Increment ref count as we are using it
       mip->refCount = 1;
       // I DONT KNOW WHAT THIS DOES <--------------------------------------------------------------------------------------------------------------------
       mip->dev = dev; mip->ino = ino;  // assing to (dev, ino)
       // WHAT DOES DIRTY MEAN <--------------------------------------------------------------------------------------------------------------------------
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
  // If this fails, there must be no more minodes, thus, inform the user
  printf("\nPANIC: no more free minodes\n");
  return 0;
}

// Initializes Procs and root+running
void init()
{
  proc[0].uid = 0;
  proc[0].pid = 0;
  proc[0].cwd = 0;

  proc[1].uid = 1;
  proc[1].pid = 0;
  proc[1].cwd = 0;

  for(int i = 0; i < NMINODES; i++)
     minode[i].refCount = 0;

  root = 0;
  running = &proc[0];
}

// Mounts the root in order to start the program
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
    root = iget(fd, 2); // NOTE!!! THIS USED TO SAY "iget(dev,2)" BUT IVE BEEN REPLACING dev WITH fd EVERYWEHER, SO, WHAT??? <--------------------------------------------------------------------
    root->mountptr = (MOUNT *)malloc(sizeof(MOUNT));
    root->mountptr->ninodes = ninodes;
    root->mountptr->nblocks = nblocks;
    root->mountptr->dev = dev;
    root->mountptr->busy = 1;
    root->mountptr->mounted_inode = root;
  }
}

// Splits pathname into items, stores in path[], returns number of path items
int tokenizePathname()
{
  char *readLine = NULL;
  size_t len = BLOCK_SIZE;
  int j = 0;
  
  getline(&readLine, &len, stdin);


  // May have to remove an initial '/'
  // Get first token
  path[0] = strtok(readLine, "/");


  while (path[j] != NULL) {
    j++;
    path[j] = strtok(NULL, "/");
  }

  return j;
}

// Splits command into items, stores in command[], returns number of path items
int tokenizeCommmand()
{
  char *readLine = NULL;
  size_t len = BLOCK_SIZE;
  int j = 0;
  
  getline(&readLine, &len, stdin);


  // May have to remove an initial '/'
  // Get first token
  command[0] = strtok(readLine, " ");


  while (command[j] != NULL) {
    j++;
    command[j] = strtok(NULL, " ");
  }

  return j;
}

void my_ls(char *name) {
  //print directory contents 
  int i;
  MINODE *mip;
  DIR *dir;
  char buf[BLKSIZE], *cp;

  if (name[0] == '/')
  {
    dev = root->dev;
  }
  else
  {
    dev = running->cwd->dev;
  }

  i = getino(dev, name);
  if (!i)
  {
    printf("Error file not found \n\n\n");
    return;
  }
  mip = iget(dev, i);


  if (mip->ino == 0x8000)
  {
    printf("%s", basename(name));
  }
  else
  {
    for (int i = 0; i <= 11; i++)
    {
      if (mip->INODE.i_block[i])
      {
        get_block(dev, mip->INODE.i_block[i], buf);
        cp = buf;
        dir = (DIR *)buf;
        while (cp < &buf[BLKSIZE])
        {
          printf("%s ", dir->name);
          cp += dir->rec_len;
          dir = (DIR *)cp;
        }
      }
    }
  }
}

void commandTable()
{
  if(strcmp(command[0], "ls") == 0)
  {
    //This is ls
    my_ls();
  }
}



////////////////////////////////////////////////////////////////////
// MAINLINE                                                       //
////////////////////////////////////////////////////////////////////

// Vars for mainline
// Name of disk to open
char *disk = "mydisk";
// Used for iterating over while loops
int i = 0;

// Mainline handles opening of disk, then calls showblock()
main(int argc, char *argv[ ]) { 
  printf("Initializing J&J EXT2 file system\n\n");
  // Initialize the Program
  init();

  // If given a diskname, use it instead of mydisk - DEFAULT: "mydisk"
  if (argc > 1) {
    disk = argv[1];
  }
  // MOUNT ROOT
  mountRoot(disk);

  // Get commands from stdin
  //MAGIC LOOP    
  while(1)
  {
    // Print prompt for user command entry
    printf("J&J EXT2FS: ");
    // Read in and tokenize Command
    numberOfCommands = tokenizeCommmand();
    if (strcmp(command[0], "quit\n") == 0)  /////////////////////////////////////////////////Should probably make this quit not have a newline after it
    {
      //User entered Quit, we should probably exit
      break;
    }
    else
    {
      if(isDebug)
      {
        printf("Debug Echo: ");
        i = 0;
        while(i < numberOfCommands)
        {
          printf("'%s' ", command[i]);
          i++;
        }
      }
    }
    // If we have reached here, the command is not quit
    // Call command table, looks up the command in command[0] and executes
    commandTable();
  }

  printf("Exiting J&J EXT2FS program! Have a nice day!");
}