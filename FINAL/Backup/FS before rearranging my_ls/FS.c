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
#define BLOC_OFFSET(block) (BLKSIZE + block-1)*BLKSIZE // Used for getino

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
char *fullpath;
// Number of path items returned by path tokenizer
int numberOfPathItems = 0;

// Unknown, copied from original main
int dev;

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

// put the block of data from the buffer (buf) into the file device (fd)
void put_block(int fd, int block, char *buf)
{
  lseek(fd, (long)(BLKSIZE*block),0);
  write(fd, buf, BLKSIZE);
}


// This is used in my_ls()
void get_inode(int fd, int ino, int inode_table,INODE *inode) {
  lseek(fd, BLOC_OFFSET(inode_table) + (ino - 1) * sizeof(INODE), 0);
  read(fd, inode, sizeof(INODE));
}



// load INODE at (dev,ino) into a minode[]; return mip->minode[]
MINODE *iget(int dev, int ino)
{
  printf("We are in the iget function \n");
  int i, blk, disp;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;
  
  for (i=0; i < NMINODES; i++){
	printf("In for loop i < NMINODES \n\n");
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
       blk  = (ino-1)/8 + InodesBeginBlock;  // iblock = Inodes start block #
       disp = (ino-1) % 8;
       printf("allocating new Minode: iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);
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

// Needed by getino
int iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
  char buf[BLKSIZE];
  int nodeIn,blockIn;
  mip->refCount--;
  if (mip->refCount > 0) return;
  if (!mip->dirty)       return;
  
  printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

  nodeIn = (mip->ino -1 ) % INODES_PER_BLOCK; // Mailman's Algorithm
  blockIn = (mip->ino -1) / INODES_PER_BLOCK + InodesBeginBlock; // Mailman's Algorithm
  get_block(mip->dev,blockIn, buf);
  ip = (INODE *)buf;
  ip += nodeIn;
  *ip = mip->INODE;
  put_block(mip->dev,blockIn,buf); // save the minode!
}

// Needed by getino
// Vars for search
char dbuf[1024];
// Searches through data blocks to find entry specified by pathname
int search(MINODE *minodePtr, char *name) {
  printf("In search-> This is what is in minodePtr: '%d,%d'", minodePtr->dev, minodePtr->ino);
  printf("\nSEARCHING FOR: %s\n", name);
  for (int i = 0; i < 12; i++) {
	 /* if (minodePtr->INODE.i_block[i] == 0)
	  {
		  printf("This is where we return 0");
		  return 0;
	  }*/

	  // get_block(fd, minodePtr->INODE.i_block[i], dbuf);  // char dbuf[1024]
	  printf("Before if(minodePtr->INODE.i_block[%d])\n", i);
	  if (minodePtr->INODE.i_block[i])
	  {
		  printf("inside if(minodePtr->INODE.i_block[%d])\n", i);
		  get_block(minodePtr->dev, minodePtr->INODE.i_block[i], dbuf);  // char dbuf[1024]
		  DIR *dp = (SUPER *)dbuf;
		  char *cp = dbuf;

		  while (cp < &dbuf[1024])
		  {
			  //use dp-> to print the DIR entries as  [inode rec_len name_len name]
			  printf("\n - DIR ENTRY - rec_len: %d, name_len: %d, name: %s", dp->rec_len, dp->name_len, dp->name);
			  if (strcmp(name, dp->name) == 0)
			  {
				  printf("\n - Name: %s == %s", name, dp->name);
				  printf("\n - Found at INODE: %d\n", dp->inode);
				  return dp->inode;
			  }
			  printf("\n - Name: %s != %s", name, dp->name);
			  cp += dp->rec_len;
			  dp = (DIR *)cp;

			  //getchar();
		  }
	  }
	 
  }
  printf(" - Not Found\n");
  return 0;
}

// Given by KC
int getino(int *dev, char *pathname)
{
  int i, ino, blk, disp,n;
  char buf[BLKSIZE];
  INODE *gip;
  MINODE *mip;

  printf("getino: dev=%d pathname=%s\n", *dev, pathname);
  printf("Right before strcmp1\n");                /////////////////////////////// Going
  if (strcmp(pathname, "/")==0)
  {
      return 2;
  }
  printf("Right before strcmp2\n");                /////////////////////////////// Hard
  if (pathname[0]=='/')
  {
    printf("Right before iget(*dev, 2)\n");                /////////////////////// With
    mip = iget(*dev, 2); ///// taking out the damn * for dev
  }
  else    
  {
    printf("Right before iget(running->cwd->dev, running->cwd->ino)\n");   /////// The
    mip = iget(running->cwd->dev, running->cwd->ino);
  }

  printf("Right before strcpy\n"); /////////////////////////////////////////////// Prints
  strcpy(buf, pathname);
  n = tokenizePathname(buf); // n = number of token strings

  for (i=0; i < n; i++){
      //printf("===========================================\n");
      //printf("getino: i=%d name[%d]=%s\n", i, i, kcwname[i]);
	  gip = &mip->INODE;
	  printf("THIS IS WHAT IS IN PATH[0]: '%s', This is what is in mip: '%d,%d'\n\n", path[0], mip->dev, mip->ino);
	  printf("This is what is in gip: '%d,%d'\n\n", gip->i_mode, gip->i_uid);
      ino = search(mip, path[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", path[i]);
         return 0;
      }
      iput(mip);
      mip = iget(*dev, ino);//need to delete * 
  }
  iput(mip);
  return ino;
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
  // For printing inode begin block
  char buf[BLKSIZE];

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
	int nblocks, ninodes, bfree, ifree;
	nblocks = sp->s_blocks_count;
	bfree = sp->s_free_blocks_count;
	ninodes = sp->s_inodes_count;
    // Read Group Descriptor block
    get_block(fd, 2, buf);
    
    // Set the Group Descriptor pointer == buf
    gp = (SUPER *)buf;
    
    // Set InodesBeginBlock
    InodesBeginBlock = gp->bg_inode_table;
    if(isDebug) 
    {
      printf("\nInodesBeginBlock: %d\n", InodesBeginBlock);
   	  // Read in the inodes from begin block
	  printf("\nPrinting InodeBeginBlock:\n-------------------------\n - InodesBeginBlock=%d\n", InodesBeginBlock);

	  // get inode start block     
	  get_block(fd, InodesBeginBlock, buf);

	  ip = (INODE *)buf + 1;         // ip points at 2nd INODE
	  
	  printf(" - mode=%4x ", ip->i_mode);
	  printf("  uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
	  printf(" - size=%d\n", ip->i_size);
	  printf(" - time=%s", ctime(&ip->i_ctime));
	  printf(" - link=%d\n", ip->i_links_count);
	  printf(" - i_block[0]=%d\n", ip->i_block[0]);
    }

    // WHAT DOES THIS DO?
    root = iget(fd, 2); // NOTE!!! THIS USED TO SAY "iget(dev,2)" BUT IVE BEEN REPLACING dev WITH fd EVERYWEHER, SO, WHAT??? <--------------------------------------------------------------------
    root->mountptr = (MOUNT *)malloc(sizeof(MOUNT));
    root->mountptr->ninodes = ninodes;
    root->mountptr->nblocks = nblocks;
    root->mountptr->dev = fd;
    root->mountptr->busy = 1;
    root->mountptr->mounted_inode = root;
	//setting running->cwd to root

	running->cwd = root; // remember this for cd
  }
}

// Splits pathname into items, stores in path[], returns number of path items
int tokenizePathname()
{
	char *readLine = NULL;
	size_t len = BLOCK_SIZE;
	int j = 0;

	// Reset path
	while (j < 32)
	{
		path[j] = NULL;
		j++;
	}
	// Reset j
	j = 0;


	if (numberOfCommands == 1)
	{

		// May have to remove an initial '/'
		// Get first token
		path[0] = strtok(command[1], "/");


		while (path[j] != NULL) {
			j++;
			path[j] = strtok(NULL, "/");
		}

		
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
  // Remove newline from end of command string if string is longer than size 0
  if(strcmp(readLine, "\n") == 0)
  {
  	readLine = '\0';
  	command[0] = "";
  	return 1;
  }
  else
  {
	if(strlen(readLine) > 1) /////////////////////////////////////////////////////Still weird when you enter just a newline
	{
	  readLine[strlen(readLine)-1] = '\0';
	}

	// May have to remove an initial '/'
	// Get first token
	command[0] = strtok(readLine, " ");


	while (command[j] != NULL) {
	  j++;
	  command[j] = strtok(NULL, " ");
	}

	return j;
  }
}

void my_ls(char *name) {
  //print directory contents 
  int i;
  MINODE *mip;
  DIR *dir;
  char buf[BLKSIZE], *cp;

  //fflush();
  printf("Moved the print up here\n");

  //fflush();
  printf("Moved the print up here\n");
  
  if(name != NULL)
  {
	if (name[0] == '/')
	{
	  fd = root->dev;
	  if(isDebug) printf("LS from root->dev: fd = %d\n", fd);
	}
	else
	{
	  fd = running->cwd->dev; ////////////////////////////////////////////////////// THIS IS CURRENTLY REDUNDANT
	  if(isDebug) printf("LS from running->cwd->dev: fd = %d & name = %s\n", fd, name);
	}
  }
  else
  {
	fd = running->cwd->dev; //////////////////////////////////////////////////////// THIS IS CURRENTLY REDUNDANT
	name = running->cwd->name;
    if(isDebug) printf("LS (NO PARAMS) from running->cwd->dev: fd = %d & name = %s\n", fd, name);
  }

  i = getino(&fd, name); ///////////////////////////////////////////////////////////changed from getino(dev, name) to getino(fd, name)
  printf("\ni = %d\n", i);

  if (!i)
  {
    printf("Error file not found \n\n\n");
    return;
  }
  printf("mip = iget(%d, %d)\n", fd, i);
  mip = iget(fd, i); ///changed from dev to fd

  if (mip->ino == 0x8000)//
  {
    printf("%s", basename(name));
  }
  else
  {
  	printf("ACTUAL OUTPUT:\n");
    for (int i = 0; i <= 11; i++)
    {    	
  	  //printf("if(mip->INODE.i_block[%d])\n", i);
      if (mip->INODE.i_block[i])
      {
        get_block(fd, mip->INODE.i_block[i], buf);
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
    my_ls(command[1]); ///////////////////////////////////////// Should pass in the entire path, as long as it is arg 2
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
    printf("\nJ&J EXT2FS: ");
    // Read in and tokenize Command
    numberOfCommands = tokenizeCommmand();
    if (strcmp(command[0], "quit") == 0)
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

  printf("Exiting J&J EXT2FS program! Have a nice day!\n");
}