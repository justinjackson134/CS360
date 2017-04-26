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
#define SYM_LINK          0120000

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
int isDebug = 0; // IF 1, enable debug prints!

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

// Buffer used in getblock
//char buf[BLKSIZE];
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
// Number of free inodes, gived by the SUPERBLOCK
int ninodes;
// Device number
int dev;
// dirname variable
char dirname_value[128];
char basename_value[128];
// bmap and imap ints
int bmap;
int imap;

// Path NUM var to make tokenize path behave differently on # of paths given
int pathNum = 1;
// Var used to tell getino to return the parent for rmdir and unlink
int FindParent = 0;
// 
int readLinkFromLs = 0;



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
  if (isDebug) printf("We are in the iget function \n");
  int i, blk, disp;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;
  
  for (i=0; i < NMINODES; i++){
	//printf("In for loop i < NMINODES \n\n");
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       if (isDebug) printf("\nfound [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
  for (i=0; i < NMINODES; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       if (isDebug) printf("\nallocating NEW minode[%d] for [%d %d]\n", i, dev, ino);  // WHAT IS DEV? EARLIER WE GET GROUP DESCRIPTOR, IT USED TO SAY DEV, BUT THAT DIDNT WORK SO I MADE IT FD
       // Increment ref count as we are using it
       mip->refCount = 1;
       // I DONT KNOW WHAT THIS DOES <--------------------------------------------------------------------------------------------------------------------
       mip->dev = dev; mip->ino = ino;  // assing to (dev, ino)
       // WHAT DOES DIRTY MEAN <--------------------------------------------------------------------------------------------------------------------------
       mip->dirty = mip->mounted = mip->mountptr = 0;
       // get INODE of ino into buf[ ]      
       blk  = (ino-1)/8 + InodesBeginBlock;  // iblock = Inodes start block #
       disp = (ino-1) % 8;
       if (isDebug) printf("allocating new Minode: iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);
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
  
  if (isDebug) printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

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
  if (isDebug) printf("In search-> This is what is in minodePtr: '%d,%d'", minodePtr->dev, minodePtr->ino);
  if (isDebug) printf("\nSEARCHING FOR: %s\n", name);
  int i;
  for (i = 0; i < 12; i++) {
	 /* if (minodePtr->INODE.i_block[i] == 0)
	  {
		  printf("This is where we return 0");
		  return 0;
	  }*/

	  // get_block(fd, minodePtr->INODE.i_block[i], dbuf);  // char dbuf[1024]
	  //printf("Before if(minodePtr->INODE.i_block[%d])\n", i);
	  if (minodePtr->INODE.i_block[i])
	  {
		  if (isDebug) printf("inside if(minodePtr->INODE.i_block[%d])\n", i);
		  get_block(minodePtr->dev, minodePtr->INODE.i_block[i], dbuf);  // char dbuf[1024]
		  DIR *dp = (SUPER *)dbuf;
		  char *cp = dbuf;

		  while (cp < &dbuf[1024])
		  {
			  //use dp-> to print the DIR entries as  [inode rec_len name_len name]
			  if (isDebug) printf("\n - DIR ENTRY - rec_len: %d, name_len: %d, name: %s", dp->rec_len, dp->name_len, dp->name);
			  if (strcmp(name, dp->name) == 0)
			  {
				  if (isDebug) printf("\n - Name: %s == %s", name, dp->name);
				  if (isDebug) printf("\n - Found at INODE: %d\n", dp->inode);
				  return dp->inode;
			  }
			  if (isDebug) printf("\n - Name: %s != %s", name, dp->name);
			  cp += dp->rec_len;
			  dp = (DIR *)cp;

			  //getchar();
		  }
	  }
	 
  }
  if (isDebug) printf(" - Not Found\n");
  return 0;
}

// Given by KC
int getino(int *dev, char *pathname)
{
  int i, ino = 0, blk, disp,n;
  char buf[BLKSIZE];
  INODE *gip;
  MINODE *mip;

  if (isDebug) printf("getino: dev=%d pathname=%s\n", *dev, pathname);
  if (isDebug) printf("Right before strcmp1\n");                /////////////////////////////// Going
  if (strcmp(pathname, "/")==0)
  {
      return 2;
  }
  if (isDebug) printf("Right before strcmp2\n");                /////////////////////////////// Hard
  if (pathname[0]=='/')
  {
    if (isDebug) printf("Right before iget(*dev, 2)\n");                /////////////////////// With
    mip = iget(*dev, 2);
  }
  else    
  {
    if (isDebug) printf("Right before iget(running->cwd->dev, running->cwd->ino)\n");   /////// The
    mip = iget(running->cwd->dev, running->cwd->ino);
  }

  if (isDebug) printf("Right before strcpy\n"); /////////////////////////////////////////////// Prints
  strcpy(buf, pathname);


  // command[1] currently equals the entire pathname, but if we get here by mkdir, we need it to contain only the parent(dirname_value)
  if(pathNum == 1)
  {
  	n = tokenizePathname(); // n = number of token strings
  }
  else // pathNum == 2
  {
  	n = tokenizePathname2();
  	if(strcmp(command[0], "link") == 0 || (strcmp(command[0], "symlink") == 0 && FindParent == 1))
  	{
  		n -= 1;
  	}
  }

  // Tells the program that we are trying to find the parent and not just the parent and child
  if(strcmp(command[0], "mkdir") == 0 || strcmp(command[0], "creat") == 0)// || strcmp(command[0], "touch") == 0)
  {
  	n -= 1;
  }

  // Tells the program if rmdir/rm/unlink is looking for the parent or child
  if((strcmp(command[0], "rmdir") == 0 && FindParent == 1) || (strcmp(command[0], "rm") == 0 && FindParent == 1) || (strcmp(command[0], "unlink") == 0 && FindParent == 1))
  {
  	n -= 1;
  }

  for (i=0; i < n; i++){
      //printf("===========================================\n");
      //printf("getino: i=%d name[%d]=%s\n", i, i, kcwname[i]);
	  gip = &mip->INODE;
	  if (isDebug) printf("THIS IS WHAT IS IN PATH[0]: '%s', This is what is in mip: '%d,%d'\n\n", path[0], mip->dev, mip->ino);
	  if (isDebug) printf("This is what is in gip: '%d,%d'\n\n", gip->i_mode, gip->i_uid);
      ino = search(mip, path[i]);

      if (ino==0){
         iput(mip);
         if (isDebug) printf("name %s does not exist\n", path[i]);
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
  int i;
  for(i = 0; i < NMINODES; i++)
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
  if(isDebug) printf("TRYING TO OPEN DISK: FD = %d\n", fd);
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

  // Added this from the ialloc.c of lab 6, I believe we need it in order for ialloc and balloc to work correctly --- This may need to be something else however!
  ninodes = sp->s_inodes_count;

  if(isDebug)
  {
    if (isDebug) printf("\nSUPERBLOCK\n-------------------------\n - nblocks:          %d\n - ninodes:          %d\n - inodes_per_group: %d\n - # free inodes:    %d\n - # free blocks:    %d\n",
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
    // Added this from the ialloc.c of lab 6, I believe we need it in order for ialloc and balloc to work correctly --- This may need to be something else however!
    imap = gp->bg_inode_bitmap;
  	if(isDebug) printf("imap = %d\n", imap);
  	bmap = gp->bg_block_bitmap;
  	if(isDebug) printf("bmap = %d\n", bmap);


    if(isDebug) 
    {
      if (isDebug) printf("\nInodesBeginBlock: %d\n", InodesBeginBlock);
   	  // Read in the inodes from begin block
	  if (isDebug) printf("\nPrinting InodeBeginBlock:\n-------------------------\n - InodesBeginBlock=%d\n", InodesBeginBlock);

	  // get inode start block     
	  get_block(fd, InodesBeginBlock, buf);

	  ip = (INODE *)buf + 1;         // ip points at 2nd INODE
	  
	  if (isDebug) printf(" - mode=%4x ", ip->i_mode);
	  if (isDebug) printf("  uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
	  if (isDebug) printf(" - size=%d\n", ip->i_size);
	  if (isDebug) printf(" - time=%s", ctime(&ip->i_ctime));
	  if (isDebug) printf(" - link=%d\n", ip->i_links_count);
	  if (isDebug) printf(" - i_block[0]=%d\n", ip->i_block[0]);
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
	size_t len = BLOCK_SIZE;
	int j = 0;
	char copyOfPathname[128];

	strcpy(copyOfPathname, command[1]);
	if (isDebug) printf("copyOfPathname: %s == command[1]: %s\n", copyOfPathname, command[1]);
	if (isDebug) printf("This is pathname to be tokenized, stored in command[1]: %s, \nThis is the number of commmands: %d\n", command[1], numberOfCommands);

	// Reset path
	while (j < 32)
	{
		path[j] = NULL;
		j++;
	}
	// Reset j
	j = 0;

	if (numberOfCommands > 1)
	{
		// Get first token
		path[0] = strtok(copyOfPathname, "/");

		while (path[j] != NULL) {
			j++;
			path[j] = strtok(NULL, "/");
		}		
	}

	// Print value of command[1]
	if (isDebug) printf("Now Command[1] = %s\n", command[1]);
	return j;
}

// Splits pathname into items, stores in path[], returns number of path items
int tokenizePathname2()
{
	size_t len = BLOCK_SIZE;
	int j = 0;
	char copyOfPathname[128];

	strcpy(copyOfPathname, command[2]);
	if (isDebug) printf("copyOfPathname: %s == command[2]: %s\n", copyOfPathname, command[2]);
	if (isDebug) printf("This is pathname to be tokenized, stored in command[2]: %s, \nThis is the number of commmands: %d\n", command[2], numberOfCommands);

	// Reset path
	while (j < 32)
	{
		path[j] = NULL;
		j++;
	}
	// Reset j
	j = 0;

	if (numberOfCommands > 1)
	{
		// Get first token
		path[0] = strtok(copyOfPathname, "/");

		while (path[j] != NULL) {
			j++;
			path[j] = strtok(NULL, "/");
		}		
	}

	// Print value of command[1]
	if (isDebug) printf("Now Command[2] = %s\n", command[2]);
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

void my_help()
{
	printf("\n\n");
	printf("--------------- J&J EXT2FS HELP ---------------\n");
	printf("Commands: ls, cd, pwd, mkdir, rmdir, creat, rm,\n");
	printf("          link, unlink, symlink, readlink,\n");
	printf("          chmod, touch, quit\n\n");
}

void my_ls(char *name) {
  //print directory contents 
  int i, ino;
  MINODE *mip, *printMe;
  DIR *dir;
  char buf[BLKSIZE], *cp;
 
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

	i = getino(&fd, name); ///////////////////////////////////////////////////////////changed from getino(dev, name) to getino(fd, name)
	if(isDebug) printf("\ni = %d\n", i);

	if (!i)
	{
		printf("Error file not found \n\n\n");
		return;
	}
	if (isDebug) printf("mip = iget(%d, %d)\n", fd, i);
	mip = iget(fd, i); ///changed from dev to fd

	if (mip->ino == 0x8000)// This is wrong
	{
		// This sets a global named basename!
		setDirnameBasename(name);
		// Print the global basename
		if (isDebug) printf("%s", basename_value);
	}
	else
	{
		if (isDebug) printf("ACTUAL OUTPUT:\n");
		int i;
		for (i = 0; i <= 11; i++)
		{
			//printf("if(mip->INODE.i_block[%d])\n", i);
			if (mip->INODE.i_block[i])
			{
				get_block(fd, mip->INODE.i_block[i], buf);
				cp = buf;
				dir = (DIR *)buf;
				printf("Name\tMode\tC_time\t\tM_time\t\tA_time\tLink Count\n");
				while (cp < &buf[BLKSIZE])
				{					
					// This was the original print // printf("%s ", dir->name);
					// Get the parents inode number
					ino = dir->inode;
					// Load the parents inode into a MINODE
					printMe = iget(fd, ino);
					// Print printMe's info
					if ((printMe->INODE.i_mode & 0120000) == 0120000)
					{
						//strncpy(command[1], dir->name, dir->name_len);
						readLinkFromLs = ino;
						read_link(dir->name);
						readLinkFromLs = 0;
					}
					else
					{
						printf("%s\t%07o\t%d\t%d\t%d\t%d\n", dir->name, printMe->INODE.i_mode, printMe->INODE.i_ctime, printMe->INODE.i_mtime, printMe->INODE.i_atime, printMe->INODE.i_links_count);					
					}

					// Put the MINODE back into fd
					iput(printMe);

					cp += dir->rec_len;
					dir = (DIR *)cp;
				}
			}
		}
	}
  }
  else
  {
	fd = running->cwd->dev; //////////////////////////////////////////////////////// THIS IS CURRENTLY REDUNDANT
	name = running->cwd->name;//i dont think we need this but i will leave it for now
    if(isDebug) printf("LS (NO PARAMS) from running->cwd->dev: fd = %d & name = %s\n", fd, name);
	mip = iget(fd, running->cwd->ino);

	if (mip->ino == 0x8000)//this needs to be changed 
	{
		// This sets a global named basename!
		setDirnameBasename(name);
		// Print the global basename
		if (isDebug) printf("%s", basename_value);
	}
	else
	{
		if (isDebug) printf("ACTUAL OUTPUT:\n");
		int i;
		for (i = 0; i <= 11; i++)
		{
			//printf("if(mip->INODE.i_block[%d])\n", i);
			if (mip->INODE.i_block[i])
			{
				get_block(fd, mip->INODE.i_block[i], buf);
				cp = buf;
				dir = (DIR *)buf;
				while (cp < &buf[BLKSIZE])
				{
					// This was the original print // printf("%s ", dir->name);
					// Get the parents inode number
					ino = dir->inode;
					// Load the parents inode into a MINODE
					printMe = iget(fd, ino);
					// Print printMe's info
					if ((printMe->INODE.i_mode & 0120000) == 0120000)
					{
						//strncpy(command[1], dir->name, dir->name_len);
						readLinkFromLs = ino;
						read_link(dir->name);
						readLinkFromLs = 0;
					}
					else
					{
						printf("%s\t%07o\t%d\t%d\t%d\t%d\n", dir->name, printMe->INODE.i_mode, printMe->INODE.i_ctime, printMe->INODE.i_mtime, printMe->INODE.i_atime, printMe->INODE.i_links_count);					
					}


					cp += dir->rec_len;
					dir = (DIR *)cp;
				}
			}
		}
	}
  }
}

void my_cd(char *pathname)
{
	MINODE *mip, *oldMip;
	int ino;

	if (isDebug) printf("\nIn my_cd(%s)\n", pathname);
	
	// Store a handle to our current MINODE before changing cwd
	if (isDebug) printf("begining storing running->cwd in oldMip\n");
	oldMip = running->cwd;
	if (isDebug) printf("completed storing running->cwd in oldMip\n");

	// If we are told to cd to root, we dont need getino
	if(pathname == NULL || strcmp(pathname, "/") == 0)
	{
		if (isDebug) printf("We were given the root, so, set cwd to root");

		// Change cwd to root
		running->cwd = iget(root->dev, ROOT_INODE);
		// Return old MINODE to fd
		iput(oldMip);
	}
	else
	{
		if (isDebug) printf("We are now checking if pathname exists");
		// Get inode from pathname
		ino = getino(&fd, pathname);
		// Check if the given pathname was not found
		if(ino == 0)
		{
			printf("The directory entered was not found\n");
			// Break early as we should not change the cwd
			return;
		}

		// Get the inode of pathname and store it in a new MINODE
		mip = iget(root->dev, ino);

		if(!S_ISDIR(mip->INODE.i_mode))
		{
			// This is not a directory, it cannot be cwd!
			iput(mip);
			printf("The entered file: '%s' is not a directory", pathname);
			// Break early as we should not change the cwd
			return;
		}

		// Change the running process cwd to our new MINODE
		running->cwd = mip;
		// Return old MINODE to fd 
		iput(mip);
	}
}

void my_pwd()
{
	// Print a newline as the pathname does not begin with a \n
	printf("\n");
	// Call the recursive_pwd function --- climb upto root, then print back down
	recursive_pwd(running->cwd, 0);
}

void recursive_pwd(MINODE *cwd, int child_ino)
{
  	char buf[BLKSIZE], *cp, name[128];
  	DIR *dir;
	int ino;
	MINODE *parentMinodePtr;

	// If we are now at the root dir, begin to print
	if(cwd->ino == root->ino)
	{
		printf("/");
	}
	
	// Get the directorory block for the current directory
	get_block(root->dev, cwd->INODE.i_block[0], buf);
	// Set dp = '.'
	dir = (DIR *)buf;
	// Advance one record
	cp = buf + dir->rec_len;
	// Set dp = '..' (This lets us go up a level)
	dir = (DIR *)cp;
	//printf("DP CURRENTLY: '%s'\n", dir->name);
	
	// If we have not reached the root directory
	if(cwd->ino != root->ino)
	{
		// Get the parents inode number
		ino = dir->inode;
		// Load the parents inode into a MINODE
		parentMinodePtr = iget(fd, ino);
		// Recurse up to the parent
		recursive_pwd(parentMinodePtr, cwd->ino);
		// Put the MINODE back into fd
		iput(parentMinodePtr);
	}

	// If the child_ino number != 0, we are still recursing
	if(child_ino != 0)
	{
		// loop until we find the correct child inode in the directory
		while(dir->inode != child_ino)
		{
			// Iterate over the current directory entry
			cp += dir->rec_len;
			dir = (DIR *)cp;
		}

		// Copy the found name into the name var for printing
		strncpy(name, dir->name, dir->name_len);
		// Set the string to be null terminated
		name[dir->name_len] = '\0';
		// Print out the name in "name/" format
		printf("%s/", name);
	}

	return;
}

///////////////////////////////////////////////////////////////////////////////////// ALL COPIED FROM LAB 6

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int decFreeInodes(int mydev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(mydev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(mydev, SUPERBLOCK, buf);

  get_block(mydev, GDBLOCK, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(mydev, GDBLOCK, buf);
}

int ialloc(int mydev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(mydev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(mydev);

       put_block(mydev, imap, buf);
	   fflush(stdout);
       //printf("!!! IALLOC returning i+1: %d\n", i+1);
       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

int decFreeBlocks(int mydev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(mydev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, SUPERBLOCK, buf);

  get_block(mydev, GDBLOCK, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(mydev, GDBLOCK, buf);
}

int balloc(int mydev)
{
  int  i;
  char buf[BLKSIZE];

  // read block_bitmap
  get_block(mydev, bmap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeBlocks(bmap);

       put_block(mydev, bmap, buf);
	   
       // GARBAGE PRINTS FROM HERE <-----------------------------------------------------------------------------------

       if (isDebug) printf("!!! BALLOC returning i+1: %d\n", i+1);
       return i+1;
    }
  }
  printf("balloc(): no more free blocks\n");
  return 0;
}

int falloc(OFT *op) {
	int i = 0;
	// Check all OFT pointers in running process
	for (i = 0; i < NFD; i++) {
		if (running->fd[i] == NULL)
			break;
	}
	// if there is a free OFT pointer
	if (i < NFD) {
		// Assign to next available location in running process
		running->fd[i] = op;
		return i;
	}
	// no OFT pointers available
	return -1;
}


void truncate(int dev, MINODE *mip)
{
	//deallocates all i_blocks of an inode
	deallocIBlocks(dev, mip);
	
	mip->INODE.i_size = 0;
	mip->INODE.i_mtime = time(NULL);
	mip->dirty = 1;

}
void idealloc(int dev, int ino)
{
	char buf[BLOCK_SIZE];
	if(isDebug) printf("Dealloc: Get_block\n");
	get_block(dev, IBITMAP, buf); //gets the ibitmap block into buf
	if(isDebug) printf("Dealloc: clr_bit\n");
	clr_bit(buf, ino - 1); //clears the inode in the map, ino-1 cause of indexes
	if(isDebug) printf("Dealloc: put_block\n");
	put_block(dev, IBITMAP, buf);
	if(isDebug) printf("Dealloc: Done\n");
}
//deallocates all blocks of an inode, used for removing 
void deallocIBlocks(int dev, MINODE *mip)
{
	char buf[BLOCK_SIZE];
	char bitmap[BLOCK_SIZE], dibuff[BLOCK_SIZE];
	int i, j, indirblk, dindirblk, *ibit, *dbit;
	for (i = 0; i < 12; i++)
	{
		if (mip->INODE.i_block[i] - 1)
		{
			clr_bit(bitmap, mip->INODE.i_block[i] - 1);
			mip->INODE.i_block[i] = 0;
		}
		else
		{
			put_block(dev, BBITMAP, bitmap);
			return;
		}
	}

	if (mip->INODE.i_block[i] != 0)
	{
		indirblk = mip->INODE.i_block[i];
		get_block(dev, indirblk, buf);
		ibit = (int *)buf;
		for (i = 0; i < 256; i++)
		{
			if (*ibit != 0)
			{
				clr_bit(bitmap, *ibit - 1);
				*ibit = 0;
				ibit++;
			}
			else
			{
				clr_bit(bitmap, indirblk - 1);
				put_block(dev, indirblk, buf);
				put_block(dev, BBITMAP, bitmap);
				mip->INODE.i_block[12] = 0;
				return;
			}
		}

	}
	else
	{
		put_block(dev, BBITMAP, bitmap);
		return;
	}

	//deallocate double indirect blocks

	if (mip->INODE.i_block[13] != 0)
	{
		dindirblk = mip->INODE.i_block[13];
		get_block(dev, dindirblk, dibuff);
		dbit = (int *)dibuff;
		for (i = 0; i < 256; i++)
		{
			indirblk = *dbit;
			get_block(dev, indirblk, buf);
			ibit = (int *)buf;
			for (j = 0; j < 256; j++)
			{
				if (*ibit != 0)
				{
					clr_bit(bitmap, *ibit - 1);
					*ibit = 0;
					ibit++;
				}
				else
				{
					clr_bit(bitmap, indirblk - 1);
					clr_bit(bitmap, dindirblk - 1);
					put_block(dev, indirblk, buf);
					put_block(dev, BBITMAP, bitmap);
					put_block(dev, dindirblk, dibuff);
					mip->INODE.i_block[13] = 0;
					return;
				}
				clr_bit(bitmap, indirblk - 1);
			}
			dbit++;
			if (*dbit == 0)
			{
				clr_bit(bitmap, indirblk - 1);
				clr_bit(bitmap, dindirblk - 1);
				put_block(dev, indirblk, buf);
				put_block(dev, BBITMAP, bitmap);
				put_block(dev, dindirblk, dibuff);
				mip->INODE.i_block[13] = 0;
				return;
			}
		}
	}
	else
	{
		put_block(dev, BBITMAP, bitmap);
		return;
	}
}

// Set both dirnmae and basename
int setDirnameBasename(char *pathname)
{
	int j = 0;
	j = tokenizePathname();
	dirname(pathname, j);
	basename(pathname, j);
}

// Set both dirnmae and basename
int setDirnameBasename2(char *pathname)
{
	int j = 0;
	j = tokenizePathname2();
	dirname(pathname, j);
	basename(pathname, j);
}

// Sets dirname global to the directory name upto but not including the final '/'
int dirname(char *pathname, int j)
{
	int i = 0;
	char out[128];	
	strcpy(out, "");
	if(pathname != NULL)
	{
		if(pathname[0] == '/')
		{
			strcat(out, "/");
		}
	}

	while(i < j-1)
	{
		if (isDebug) printf("Piece: %s\n", path[i]);
		strcat(out, path[i]);
		if (i != j-2)
		{
			strcat(out, "/");
		}
		i++;
	}

	if (isDebug) printf("Setting dirname_value = %s\n", out);
	strcpy(dirname_value, out);

	return j;
}

// Sets basename global to everything after the final '/' of pathname
int basename(char *pathname, int j)
{
	char out[128];
	strcpy(out, "");

	// If the path is not null, set basename
	if(path[j-1] != NULL)
	{
		strcpy(out, path[j-1]);
	}

	if (isDebug) printf("Setting basename_value = %s\n", out);
	strcpy(basename_value, out);
}

int my_make_dir(char *pathname)
{
  MINODE *parentMinodePtr;
  char *parent, *child;
  int parentInode;
  int isRootPath = 0;
 
  if(pathname != NULL)
  {
	if (pathname[0] == '/')
	{
	  fd = root->dev;
	  if(isDebug) printf("MKDIR from root->dev: fd = %d\n", fd);
	  isRootPath = 1;
	}
	else
	{
	  fd = running->cwd->dev;
	  if(isDebug) printf("MKDIR from running->cwd->dev: fd = %d\n", fd);
	  isRootPath = 0;
	}

	// Set dirname and basename globals given pathname
	setDirnameBasename(pathname);

	// Set the parent and child equal to the new dirname/basename globals
	parent = dirname_value;
	child = basename_value;
	if (isDebug) printf("RAW: Parent: %s\nChild: %s\n", parent, child);

	if(strcmp(parent, "") == 0)
	{
		if(isRootPath == 1)
		{
			// Parent is null, but we are a root path, set parent == root
			parent = "/";
		}
	}	

	if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);

	// If the child is null, we cannot create this directory
	if(strcmp(child, "") == 0 || child == NULL)
	{
		printf("Cannot Create Empty Directory!\n");
		return;
	}

	// Get the inode number of the parent MINODE
	if (isDebug) printf("Setting parentInode\n");
	parentInode = getino(&root->dev, parent);

	if(strcmp(parent, "") == 0)
	{		
		if (isRootPath == 0)
		{
			// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
			parentInode = running->cwd->ino;
		}
	}

	// Check if parent inode does not exist
	if (parentInode == 0)
	{		
		printf("The Given Path Contains a non-existant directory\n");
		return;
	}
	
	// Get the In_MEMORY minode of parent:
	if (isDebug) printf("Setting parentMinodePtr\n");
	parentMinodePtr = iget(root->dev, parentInode);

	// Check if the parent minode is a dir
	if (isDebug) printf("Checking if S_ISDIR\n");
	if(S_ISDIR(parentMinodePtr->INODE.i_mode))
	{
		// Make sure the child does not already exist
		if(search(parentMinodePtr, child) == 0)
		{	
			// Call mkdir helper function
			if (isDebug) printf("Calling mkdir helper\n");
			my_make_dir_Helper(parentMinodePtr, child);
			// Increment the parents link count
			parentMinodePtr->INODE.i_links_count++;
			parentMinodePtr->INODE.i_atime = time(0);
		}
		else
		{
			printf("\nTarget Already Exists, cannot mkdir\n");
		}
	}
	else
	{
		printf("\nCannot mkdir, below a file\n");
	}
  }
  else
  {
  	printf("\nMkdir missing pathname parameter\n");
  }
}

int my_make_dir_Helper(MINODE *parentMinodePtr, char *name)
{
	int ino, bno;
	MINODE *mip;
  	char buf[BLKSIZE];
  	char *cp;

  	// NOTE! as we are adding this dir entry to the parent, we must be pointing at the parents dev id // THIS MAY BE WRONG
  	fd = parentMinodePtr->dev;

  	if (isDebug) printf("Allocating ino and bno on fd = %d\n", fd);
	ino = ialloc(fd);
	bno = balloc(fd);
	if (isDebug) printf("After allocation, ino = %d, bno = %d\n", ino, bno);

	if (isDebug) printf("Pointing mip at ino\n");
	mip = iget(fd,ino);
	// WE NEED TO INITIALIZE MIP
	mip->INODE.i_block[0] = bno;

	ip = &mip->INODE;
	if (isDebug) printf("ip points at &mip->INODE, mip->ino = %d\n", mip->ino);

	// Use ip-> to acess the INODE fields:
	ip->i_mode = 0x41ED;		      // OR 040755: DIR type and permissions
	ip->i_uid  = running->uid;	      // Owner uid 
	ip->i_gid  = running->gid;	      // Group Id
	ip->i_size = BLKSIZE;		      // Size in bytes 
	ip->i_links_count = 2;	          // Links count=2 because of . and ..
	ip->i_atime = time(0L);           // set to current time 
	ip->i_ctime = time(0L);           // set to current time 
	ip->i_mtime = time(0L);           // set to current time
	ip->i_blocks = 2;                 // LINUX: Blocks count in 512-byte chunks 
	ip->i_block[0] = bno;             // new DIR has one data block   
	ip->i_block[1] = 0;
	ip->i_block[2] = 0;
	ip->i_block[3] = 0;
	ip->i_block[4] = 0;
	ip->i_block[5] = 0;
	ip->i_block[6] = 0;
	ip->i_block[7] = 0;
	ip->i_block[8] = 0;
	ip->i_block[9] = 0;
	ip->i_block[10] = 0;
	ip->i_block[11] = 0;
	ip->i_block[12] = 0;
	ip->i_block[13] = 0;
	ip->i_block[14] = 0;
	 
	mip->dirty = 1;                   // mark minode dirty
	iput(mip);                        // write INODE to disk

	// Start creating the buf to be our new dir entry
	// initialize buf to all 0's
	memset(buf, 0, BLKSIZE);
	// Setup dp pointer
	dp = (DIR *)buf;

	// Add '.' directory
	// point to the inode we just allocated
	if (isDebug) printf("Adding '.' directory to new dir\n");
	dp->inode = ino;
	strncpy(dp->name, ".", 1);
	dp->name_len = 1;
	dp->rec_len = 12;
	if (isDebug) printf("Results: dp->inode = %d, dp->name = %s, dp->name_len = %d, dp->rec_len = %d\n", dp->inode, dp->name, dp->name_len, dp->rec_len);

	// advance one dir place
	cp = buf;
	cp += dp->rec_len;
	dp = (DIR *) cp;

	// Add '..' directory	
	if (isDebug) printf("Adding '..' directory to new dir\n");
	dp->inode = parentMinodePtr->ino;
	strncpy(dp->name, "..", 2);
	dp->name_len = 2;
	dp->rec_len = BLOCK_SIZE - 12; // This needs to be equal to the remaining space on the block!
	if (isDebug) printf("Results: dp->inode = %d, dp->name = %s, dp->name_len = %d, dp->rec_len = %d\n", dp->inode, dp->name, dp->name_len, dp->rec_len);

	// Put the block into the file system
	put_block(fd, bno, buf);

	// get the parent MINODES block into buf (CALLED IN NEXT FUNCTION)
	//get_block(fd, parentMinodePtr->INODE.i_block[0], buf);

	// We never set mips name! We may not be initializing this as much as needed!!!!
	if (isDebug) printf("Going to Enter_Name: name = %s\n", name);
	enter_name(parentMinodePtr, mip->ino, name);
}

int enter_name(MINODE *parentMinodePtr, int myino, char *myname)
{
	char *cp;
	int i = 0;
	int need_length = 0, last_length = 0, last_ideal = 0;
  	char buf[BLKSIZE];

	if (isDebug) printf("Inside ofEnter_Name: name = %s\n", myname);

  	// get the parent MINODES block into buf
	get_block(fd, parentMinodePtr->INODE.i_block[0], buf);

	// Setup cp and dp
	cp = buf;
	dp = (DIR *)buf;

	// Step to the end of the data block
    if (isDebug) printf("step to LAST entry in data block %d\n", buf);
	while (cp + dp->rec_len < buf + BLKSIZE)
	{
		if (isDebug) printf("Stepping Over: %s\n", dp->name);
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
	if (isDebug) printf("Ended on: %s\n", dp->name);

	// Calculate needed length of the last record entry	
	last_ideal = 4*( (8 + dp->name_len + 3 ) / 4 ); 
	if (isDebug) printf("last_ideal = %d\n", last_ideal);
	// Calculate and store the length of the new dir item
	last_length = dp->rec_len - last_ideal; // Last_length = current record length - its ideal length = the amount left after changing it to ideal length
	if (isDebug) printf("last_length = %d\n", last_length);

	if (isDebug) printf("Checking if we have enough space on the current block...\n");
	// Check if we have enough space in this block to add our record
	if(last_length >= last_ideal)
	{
		if (isDebug) printf("NA: We have enough space\n");
		// If we have space, change the last entries length to last_ideal
		dp->rec_len = last_ideal;

		// Advance one more position
		cp += dp->rec_len;
		dp = (DIR *)cp;

		// Setup our DIR entry values
		dp->rec_len = last_length;
		dp->name_len = strlen(myname);
		dp->inode = myino;
		strncpy(dp->name, myname, strlen(myname));

		if (isDebug) printf("NA: Creating new dp->, rec_len = %d, name_len = %d, inode = %d, name = %s\n", dp->rec_len, dp->name_len, dp->inode, dp->name);

		// Write this block back to fd
		if (isDebug) printf("NA: Writing this block to fd\n");
		put_block(fd, parentMinodePtr->INODE.i_block[0], buf);
	}
	// Otherwise, allocate a new block if needed
	else
	{
		if (isDebug) printf("AL: We do NOT have enough space\n");
		i = 0;
		while(last_length < last_ideal)
		{
			i++;
			// If this block == 0, allocate a new one
			if(parentMinodePtr->INODE.i_block[i] == 0)
			{
				parentMinodePtr->INODE.i_block[i] = balloc(fd);
				parentMinodePtr->refCount = 0;
				// Now that we have a new block, the size is BLKSIZE
				last_length = BLKSIZE;
				// Wipe our buffer
				memset(buf, 0, BLKSIZE);
				// Point dp 
				dp = (DIR *)buf;
			}
			else
			{
				// get the parent MINODES block into buf
				get_block(fd, parentMinodePtr->INODE.i_block[i], buf);

				// Setup cp and dp
				cp = buf;
				dp = (DIR *)buf;

				// Step to the end of the data block
			    if (isDebug) printf("AL: step to LAST entry in data block %d\n", buf);
				while (cp + dp->rec_len < buf + BLKSIZE)
				{
					if (isDebug) printf("AL: Stepping Over: %s\n", dp->name);
					cp += dp->rec_len;
					dp = (DIR *)cp;
				}
				if (isDebug) printf("AL: Ended on: %s\n", dp->name);

				// Calculate needed length of the last record entry	
				last_ideal = 4*( (8 + dp->name_len + 3 ) / 4 ); 
				if (isDebug) printf("AL: last_ideal = %d\n", last_ideal);
				// Calculate and store the length of the new dir item
				last_length = dp->rec_len - last_ideal; // Last_length = current record length - its ideal length = the amount left after changing it to ideal length
				if (isDebug) printf("AL: last_length = %d\n", last_length);

				if (isDebug) printf("AL: Checking if we have enough space on the current block...\n");
				// Check if we have enough space in this block to add our record
				if(last_length >= last_ideal)
				{
					if (isDebug) printf("AL: We have enough space\n");
					// If we have space, change the last entries length to last_ideal
					dp->rec_len = last_ideal;

					// Advance one more position
					cp += dp->rec_len;
					dp = (DIR *)cp;
				}
			}
		}

		// Setup our DIR entry values
		dp->rec_len = last_length;
		dp->name_len = strlen(myname);
		dp->inode = myino;
		strncpy(dp->name, myname, strlen(myname));

		if (isDebug) printf("AL: Creating new dp->, rec_len = %d, name_len = %d, inode = %d, name = %s\n", dp->rec_len, dp->name_len, dp->inode, dp->name);

		// Write this block back to fd
		if (isDebug) printf("AL: Writing this block to fd\n");
		put_block(fd, parentMinodePtr->INODE.i_block[i], buf);
	}

	parentMinodePtr->dirty = 1;
	parentMinodePtr->refCount++;
	parentMinodePtr->INODE.i_atime = time(0L);
	iput(parentMinodePtr);

	return myino;
}

int my_creat(char *pathname)
{
  MINODE *parentMinodePtr;
  char *parent, *child;
  int parentInode;
  int isRootPath = 0;
 
  if(pathname != NULL)
  {
	if (pathname[0] == '/')
	{
	  fd = root->dev;
	  if(isDebug) printf("CREAT from root->dev: fd = %d\n", fd);
	  isRootPath = 1;
	}
	else
	{
	  fd = running->cwd->dev;
	  if(isDebug) printf("CREAT from running->cwd->dev: fd = %d\n", fd);
	  isRootPath = 0;
	}

	// Set dirname and basename globals given pathname
	if(pathNum == 1)
  	{
		setDirnameBasename(pathname);
  	}
  	else // pathNum == 2
  	{  		
		setDirnameBasename2(pathname);
  	}


	// Set the parent and child equal to the new dirname/basename globals
	parent = dirname_value;
	child = basename_value;
	if (isDebug) printf("RAW: Parent: %s\nChild: %s\n", parent, child);

	if(strcmp(parent, "") == 0)
	{
		if(isRootPath == 1)
		{
			// Parent is null, but we are a root path, set parent == root
			parent = "/";
		}
	}	

	if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);

	// If the child is null, we cannot create this directory
	if(strcmp(child, "") == 0 || child == NULL)
	{
		printf("Cannot Create Unnamed File!\n");
		return;
	}

	// Get the inode number of the parent MINODE
	if (isDebug) printf("Setting parentInode\n");
	parentInode = getino(&root->dev, parent);

	if(strcmp(parent, "") == 0)
	{		
		if (isRootPath == 0)
		{
			// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
			parentInode = running->cwd->ino;
		}
	}

	// Check if parent inode does not exist
	if (parentInode == 0)
	{		
		printf("The Given Path Contains a non-existant directory\n");
		return;
	}
	
	// Get the In_MEMORY minode of parent:
	if (isDebug) printf("Setting parentMinodePtr\n");
	parentMinodePtr = iget(root->dev, parentInode);

	// Check if the parent minode is a dir
	if (isDebug) printf("Checking if S_ISDIR\n");
	if(S_ISDIR(parentMinodePtr->INODE.i_mode))
	{
		// Make sure the child does not already exist
		if(search(parentMinodePtr, child) == 0)
		{	
			// Call mkdir helper function
			if (isDebug) printf("Calling CREAT helper\n");
			my_creat_helper(parentMinodePtr, child);
		}
		else
		{
			printf("\nTarget Already Exists, cannot CREAT\n");
		}
	}
	else
	{
		printf("\nCannot CREAT, below a file\n");
	}
  }
  else
  {
  	printf("\nCREAT missing pathname parameter\n");
  }
}

int my_creat_helper(MINODE* parentMinodePtr, char *name)
{
	int ino, bno;
	MINODE *mip;
  	char buf[BLKSIZE];
  	char *cp;

  	// NOTE! as we are adding this dir entry to the parent, we must be pointing at the parents dev id // THIS MAY BE WRONG
  	fd = parentMinodePtr->dev;

  	if (isDebug) printf("Allocating ino fd = %d\n", fd);
	ino = ialloc(fd);
	//bno = balloc(fd);
	if (isDebug) printf("After allocation, ino = %d\n", ino);

	if (isDebug) printf("Pointing mip at ino\n");
	mip = iget(fd,ino);
	// WE NEED TO INITIALIZE MIP
	mip->INODE.i_block[0] = 0;
	mip->INODE.i_block[1] = 0;
	mip->INODE.i_block[2] = 0;
	mip->INODE.i_block[3] = 0;
	mip->INODE.i_block[4] = 0;
	mip->INODE.i_block[5] = 0;
	mip->INODE.i_block[6] = 0;
	mip->INODE.i_block[7] = 0;
	mip->INODE.i_block[8] = 0;
	mip->INODE.i_block[9] = 0;
	mip->INODE.i_block[10] = 0;
	mip->INODE.i_block[11] = 0;
	mip->INODE.i_block[12] = 0;
	mip->INODE.i_block[13] = 0;
	mip->INODE.i_block[14] = 0;
	mip->INODE.i_block[15] = 0;

	ip = &mip->INODE;
	if (isDebug) printf("ip points at &mip->INODE, mip->ino = %d\n", mip->ino);

	// Use ip-> to acess the INODE fields:
	ip->i_mode = 0x81A4;		      // OR 040755: FILE type and permissions
	ip->i_uid  = running->uid;	      // Owner uid 
	ip->i_gid  = running->gid;	      // Group Id
	ip->i_size = 0;       		      // Size in bytes 
	ip->i_links_count = 1;	          // Links count=1 because file
	ip->i_atime = time(0L);           // set to current time 
	ip->i_ctime = time(0L);           // set to current time 
	ip->i_mtime = time(0L);           // set to current time
	ip->i_blocks = 2;                 // LINUX: Blocks count in 512-byte chunks 
	 
	mip->dirty = 1;                   // mark minode dirty
	iput(mip);                        // write INODE to disk

	// get the parent MINODES block into buf (CALLED IN NEXT FUNCTION)
	//get_block(fd, parentMinodePtr->INODE.i_block[0], buf);

	// We never set mips name! We may not be initializing this as much as needed!!!!
	if (isDebug) printf("Going to Enter_Name: name = %s\n", name);
	enter_name(parentMinodePtr, mip->ino, name);
}

int my_rm_dir(char *pathname)
{

  MINODE *parentMinodePtr, *childMinodePtr;
  char *parent, *child;
  int parentInode, childInode;
  int isRootPath = 0;
 
  if(pathname != NULL)
  {
	if (pathname[0] == '/')
	{
	  fd = root->dev;
	  if(isDebug) printf("RMDIR from root->dev: fd = %d\n", fd);
	  isRootPath = 1;
	}
	else
	{
	  fd = running->cwd->dev;
	  if(isDebug) printf("RMDIR from running->cwd->dev: fd = %d\n", fd);
	  isRootPath = 0;
	}

	// Set dirname and basename globals given pathname
	setDirnameBasename(pathname);

	// Set the parent and child equal to the new dirname/basename globals
	parent = dirname_value;
	child = basename_value;
	if (isDebug) printf("RAW: Parent: %s\nChild: %s\n", parent, child);

	if(strcmp(parent, "") == 0)
	{
		if(isRootPath == 1)
		{
			// Parent is null, but we are a root path, set parent == root
			parent = "/";
		}
	}	

	if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);

	// If the child is null, we cannot remove this directory
	if(strcmp(child, "") == 0 || child == NULL)
	{
		printf("Cannot Remove non-existant Directory!\n");
		return;
	}

	// Get the inode number of the parent MINODE
	FindParent = 1;
	parentInode = getino(&root->dev, parent);
	FindParent = 0;
	if (isDebug) printf("\n\n_________________________________\nSetting parentInode: %s, %d\n", parent, parentInode);
	

	if(strcmp(parent, "") == 0)
	{		
		if (isRootPath == 0)
		{
			// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
			parentInode = running->cwd->ino;
		}
	}

	// Check if parent inode does not exist
	if (parentInode == 0)
	{		
		printf("The Given Path Contains a non-existant directory\n");
		return;
	}

	// Get the In_MEMORY minode of parent:
	if (isDebug) printf("Setting parentMinodePtr\n");
	parentMinodePtr = iget(root->dev, parentInode);

	// Get the inode number of the child MINODE
	//childInode = getino(&parentMinodePtr, child);
	childInode = search(parentMinodePtr, child);
	if (isDebug) printf("\n\n_________________________________\nSetting childInode: %s, %d\n", child, childInode);

	// Check if child directory does not exist
	if (childInode == 0)
	{		
		printf("The Given Target does not exist\n");
		return;
	}	
	
	// set child Minodeptr
	if (isDebug) printf("Setting childMinodePtr\n");
	childMinodePtr = iget(root->dev, childInode);

	// Check if the parent minode is a dir
	if (isDebug) printf("Checking if parent S_ISDIR\n");
	if(S_ISDIR(parentMinodePtr->INODE.i_mode))
	{
		if (isDebug) printf("SEARCHING: %s <for> %s\n",parent, child);

		// Make sure the child does already exists
		if(childInode != 0)
		{	
			// RMDIR cannot remove a file that is not a directory
			if (isDebug) printf("Checking if child S_ISDIR\n");
			if(S_ISDIR(childMinodePtr->INODE.i_mode))
			{
				if(isDebug) printf("Checking Dir is Empty\n");
				if(dirIsEmpty(childMinodePtr))
				{
					if(isDebug) printf("Deallocating Child Inode: %d\n", childMinodePtr->ino);
					childMinodePtr->dirty = 1;
					idealloc(fd, childMinodePtr->ino);
					//deallocIBlocks(fd, childMinodePtr); // Deallocing blocks here ruins EVERYTHING

					// Call rmdir helper function
					if (isDebug) printf("Calling rmdir helper\n");
					my_rm_dir_Helper(parentMinodePtr, child);
				}
				else
				{
					printf("Cannot delete a non-empty directory!\n");
				}
			}
			else
			{
				printf("\nTarget is a file, not a directory\n");
			}
		}
		else
		{
			printf("\nTarget Doesnt't Exist, cannot rmdir\n");
		}
	}
	else
	{
		printf("\nCannot rmdir, below a file\n");
	}
  }
  else
  {
  	printf("\nRmdir missing pathname parameter\n");
  }
}

int my_rm(char *pathname)
{

  MINODE *parentMinodePtr, *childMinodePtr;
  char *parent, *child;
  int parentInode, childInode;
  int isRootPath = 0;
 
  if(pathname != NULL)
  {
	if (pathname[0] == '/')
	{
	  fd = root->dev;
	  if(isDebug) printf("RM from root->dev: fd = %d\n", fd);
	  isRootPath = 1;
	}
	else
	{
	  fd = running->cwd->dev;
	  if(isDebug) printf("RM from running->cwd->dev: fd = %d\n", fd);
	  isRootPath = 0;
	}

	// Set dirname and basename globals given pathname
	setDirnameBasename(pathname);

	// Set the parent and child equal to the new dirname/basename globals
	parent = dirname_value;
	child = basename_value;
	if (isDebug) printf("RAW: Parent: %s\nChild: %s\n", parent, child);

	if(strcmp(parent, "") == 0)
	{
		if(isRootPath == 1)
		{
			// Parent is null, but we are a root path, set parent == root
			parent = "/";
		}
	}	

	if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);

	// If the child is null, we cannot remove this directory
	if(strcmp(child, "") == 0 || child == NULL)
	{
		printf("Cannot Remove non-existant file!\n");
		return;
	}

	// Get the inode number of the parent MINODE
	FindParent = 1;
	parentInode = getino(&root->dev, parent);
	FindParent = 0;
	if (isDebug) printf("\n\n_________________________________\nSetting parentInode: %s, %d\n", parent, parentInode);
	

	if(strcmp(parent, "") == 0)
	{		
		if (isRootPath == 0)
		{
			// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
			parentInode = running->cwd->ino;
		}
	}

	// Check if parent inode does not exist
	if (parentInode == 0)
	{		
		printf("The Given Path Contains a non-existant directory\n");
		return;
	}

	// Get the In_MEMORY minode of parent:
	if (isDebug) printf("Setting parentMinodePtr\n");
	parentMinodePtr = iget(root->dev, parentInode);

	// Get the inode number of the child MINODE
	//childInode = getino(&parentMinodePtr, child);
	childInode = search(parentMinodePtr, child);
	if (isDebug) printf("\n\n_________________________________\nSetting childInode: %s, %d\n", child, childInode);

	// Check if child directory does not exist
	if (childInode == 0)
	{		
		printf("The Given Target does not exist\n");
		return;
	}	
	
	// set child Minodeptr
	if (isDebug) printf("Setting childMinodePtr\n");
	childMinodePtr = iget(root->dev, childInode);

	// Check if the parent minode is a dir
	if (isDebug) printf("Checking if parent S_ISDIR\n");
	if(S_ISDIR(parentMinodePtr->INODE.i_mode))
	{
		if (isDebug) printf("SEARCHING: %s <for> %s\n",parent, child);

		// Make sure the child does already exists
		if(childInode != 0)
		{	
			// RMDIR cannot remove a file that is not a directory
			if (isDebug) printf("Checking if child S_ISREG\n");
			if(S_ISREG(childMinodePtr->INODE.i_mode))
			{
				if(isDebug) printf("Deallocating Child Inode: %d\n", childMinodePtr->ino);
				childMinodePtr->dirty = 1;
				idealloc(fd, childMinodePtr->ino);
				//deallocIBlocks(fd, childMinodePtr); // Deallocing blocks here ruins EVERYTHING

				// Call rmdir helper function
				if (isDebug) printf("Calling rmdir helper\n");
				my_rm_dir_Helper(parentMinodePtr, child);				
			}
			else
			{
				printf("\nTarget is not a file\n");
			}
		}
		else
		{
			printf("\nTarget Doesnt't Exist, cannot rm\n");
		}
	}
	else
	{
		printf("\nCannot rm below a file\n");
	}
  }
  else
  {
  	printf("\nRm missing pathname parameter\n");
  }
}

void my_rm_dir_Helper(MINODE *parentMinodePtr, char *name)
{
	int temp, lastRec, distanceFromBegin = 0;
	MINODE *mip;
  	char buf[BLKSIZE];
  	char *cp, *endCP;

  	// NOTE! as we are adding this dir entry to the parent, we must be pointing at the parents dev id // THIS MAY BE WRONG
  	fd = parentMinodePtr->dev;

  	// get the parent MINODES i_block into buf so we can delete from it
  	get_block(fd, parentMinodePtr->INODE.i_block[0], buf);
  	if(isDebug) printf("ParentMinodePtr ino = %d\n", parentMinodePtr->ino);

  	// Setup cp, endCP and dp
	cp = buf;
	endCP = buf;
	dp = (DIR *)buf;

	if (isDebug) printf("getting pointer to end of buffer\n");
	while (endCP + dp->rec_len < buf + BLKSIZE)
	{
		if (isDebug) printf("GettingENDCP: %d +  < %d + %d\n", endCP, buf, BLKSIZE);
		endCP += dp->rec_len;
		if (isDebug) printf("Getting endCP: %s\n", dp->name);
		dp = (DIR *) endCP;
	}

	// Point the dp at the cp pointer -- aka the beginning of the buf
	dp = (DIR *) cp;

	// Step to the end of the data block
    if (isDebug) printf("step through data block to find: %s\n", name);
	while (cp <= buf + BLKSIZE)
	{
		if (isDebug) printf("If dp->name: %s, == name: %s\n", dp->name, name);
		if(strcmp(dp->name, name) == 0)
		{
			if (isDebug) printf("Ended on: %s\n", dp->name);
			if (isDebug) printf("Found the rec to delete, name: %s, rec_len: %d\n", dp->name, dp->rec_len);
			// Used to fix rec_lens
			temp = dp->rec_len;
			// We are deleting the last node, we need a handle to the prior node to increment its length
			if(cp == endCP)
			{
				if(isDebug) printf("We are deleting the last node\n");
				dp = (DIR *)lastRec;
				// increment the length of the last entry
				dp->rec_len += temp;
				break;
			}
			// We are deleting from the middle
			else
			{
				if(isDebug) printf("We are not deleting the last node- dp->name: %s\n", dp->name);
				dp = (DIR *) endCP;
				if(isDebug) printf("dp->name: %s\n", dp->name);
				// increment the length of the last entry
				
				if(isDebug) printf("dp->rec_len: %d + temp: %d\n", dp->rec_len, temp);
				dp->rec_len += temp;
				if(isDebug) printf("dp->rec_len: %d\n", dp->rec_len);

				// We need to move all items downstream from the deleted item up by the length of the deleted dir
				memcpy(cp, cp + temp, BLKSIZE - distanceFromBegin - temp);
			}
			break;
		}
		// Store the last cp address before advancing it -- used to adjust rec_len
		lastRec = (int) cp;
		// Keep track of how far weve moved
		distanceFromBegin += dp->rec_len;
		// Advance dp ptr
		if (isDebug) printf("Stepping Over: %s\n", dp->name);
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

	// write the changes back to the fd
	put_block(fd, parentMinodePtr->INODE.i_block[0], buf);
}

int dirIsEmpty(MINODE *childMinodePtr)
{
	// A dir is empty if it contains only '.' and '..' this means that the rec_length of .. should =BLKSIZE - 12 (12 being the size of .)
	char *cp;
	char buf[BLKSIZE];

	get_block(fd, childMinodePtr->INODE.i_block[0], buf);

	// Setup cp and dp (points at .)
	cp = buf;
	dp = (DIR *)buf;
	// Advance dp (now will point at ..)
	cp += dp->rec_len;
	dp = (DIR *)cp;

	if(dp->rec_len == BLKSIZE-12)
	{
		return 1; // Dir contains only '.' and '..'
	}
	return 0; // Dir contains more than '.' and '..'
}

void my_link(char *oldPath, char *newPath)
{
	char *parent, *child;
  	int parentInode, childInode;
  	int isRootPath = 0;
  	MINODE *Omip, *Nmip;

	if (isDebug) printf("BEGIN my_link\n");

	if (isDebug) printf("\nNUMBER OF COMMANDS: %d\n\n", numberOfCommands);
	if(numberOfCommands < 3)
	{
		printf("Not enough Parameters entered for link");
		return;
	}
	// Load Omip
	if (isDebug) printf("Loading Omip\n");
	if(oldPath != NULL)
	{
		if(oldPath[0] == '/')
		{
		  fd = root->dev;
		  if(isDebug) printf("OLDLink from root->dev: fd = %d\n", fd);
		  isRootPath = 1;
		}
		else
		{
		  fd = running->cwd->dev;
		  if(isDebug) printf("OLDLink from running->cwd->dev: fd = %d\n", fd);
		  isRootPath = 0;
		}
		// Set dirname and basename globals from pathname
		setDirnameBasename(oldPath);
		// Set parent and child equal to the new dirname/basename
		parent = dirname_value;
		child = basename_value;
		if (isDebug) printf("OLD RAW: Parent: %s\nChild: %s\n", parent, child);

		// If the parent is null but we are a root path, parent must be set to /
		if(strcmp(parent, "") == 0)
		{
			if(isRootPath == 1)
			{
				// Parent is null, but we are a root path, set parent == root
				parent = "/";
			}
		}	
		if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);
		// If the child is null, we cannot link this
		if(strcmp(child, "") == 0 || child == NULL)
		{
			printf("Cannot Create Link to nothing!\n");
			return;
		}
		if (isDebug) printf("Setting parentInode\n");
		parentInode = getino(&root->dev, parent);
		// If the parent name is empty, and were not a root path, set the parent to cwd
		if(strcmp(parent, "") == 0)
		{		
			if (isRootPath == 0)
			{
				// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
				parentInode = running->cwd->ino;
			}
		}
		// Check if parent inode does not exist
		if (parentInode == 0)
		{		
			printf("The Given Path Contains a non-existant directory\n");
			return;
		}
		
		// Get the In_MEMORY minode of parent:
		if (isDebug) printf("Setting parentMinodePtr\n");
		// Set Omip
		Omip = iget(root->dev, parentInode);
		childInode = search(Omip, child);

		// Check if child directory does not exist
		if (childInode == 0)
		{		
			printf("The Given Target does not exist\n");
			return;
		}	
		
		// set child Minodeptr
		if (isDebug) printf("Setting childMinodePtr\n");
		Omip = iget(root->dev, childInode);
	}
	if (isDebug) printf("Loaded Omip\n");

	// If Omip is a directory, we cant link to it!
	if(isDebug) printf("Omip->INODE.i_mode: %d\n", Omip->INODE.i_mode);
//	if (S_ISDIR(Omip->INODE.i_mode))
//	{
//		printf("Cannont link to a directory, returning to main menu\n");
//		return;
//	}

	if (isDebug) printf("Loading Nmip\n");
	if(newPath != NULL)
	{
		if(newPath[0] == '/')
		{
		  fd = root->dev;
		  if(isDebug) printf("OLDLink from root->dev: fd = %d\n", fd);
		  isRootPath = 1;
		}
		else
		{
		  fd = running->cwd->dev;
		  if(isDebug) printf("OLDLink from running->cwd->dev: fd = %d\n", fd);
		  isRootPath = 0;
		}
		// Set dirname and basename globals from pathname
		setDirnameBasename2(newPath);
		// Set parent and child equal to the new dirname/basename
		parent = dirname_value;
		child = basename_value;
		if (isDebug) printf("OLD RAW: Parent: %s\nChild: %s\n", parent, child);

		// If the parent is null but we are a root path, parent must be set to /
		if(strcmp(parent, "") == 0)
		{
			if(isRootPath == 1)
			{
				// Parent is null, but we are a root path, set parent == root
				parent = "/";
			}
		}	
		if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);
		// If the child is null, we cannot link this
		if(strcmp(child, "") == 0 || child == NULL)
		{
			printf("Cannot Create Link to nothing!\n");
			return;
		}
		if (isDebug) printf("Setting parentInode\n");
		pathNum = 2;
		parentInode = getino(&root->dev, parent);
		pathNum = 1;
		// If the parent name is empty, and were not a root path, set the parent to cwd
		if(strcmp(parent, "") == 0)
		{		
			if (isRootPath == 0)
			{
				// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
				parentInode = running->cwd->ino;
			}
		}
		// Check if parent inode does not exist
		if (parentInode == 0)
		{		
			printf("The Given Path Contains a non-existant directory\n");
			return;
		}
		
		// Get the In_MEMORY minode of parent:
		if (isDebug) printf("Setting parentMinodePtr\n");
		// Set Nmip
		Nmip = iget(root->dev, parentInode);
	}
	if (isDebug) printf("Loaded Nmip\n");
	// If Omip is a directory, we cant link to it!
	
	if (Nmip->INODE.i_mode == FILE_MODE || Nmip->INODE.i_mode == SYM_LINK)
	{
		printf("Cannot create new file inside a file, returning to main menu\n");
		return;
	}

	if(isDebug) printf("Searching for %s in %s\n", basename_value, dirname_value);
	int i = search(Nmip, basename_value);
	if(isDebug) printf("i = %d\n", i);

	if (i != 0)
	{
		printf("File name already exists, returning to main menu\n");
		return;
	}
	else
	{
		if (isDebug) printf("Inside else statement, calling enter_name function\n");
		enter_name(Nmip, Omip->ino, basename_value);
		if (isDebug) printf("Putting back Nmip\n");
		iput(Nmip);
		if (isDebug) printf("Incrementing Omip->INODE.i_links_count\n");
		Omip->INODE.i_links_count++;
		if (isDebug) printf("Putting back Omip\n");
		iput(Omip);
		return;
	}
}

void my_unlink(char *pathToUnlink)
{
	char *parent, *child;
  	int parentInode, childInode;
  	int isRootPath = 0;
	MINODE *mip, *pmip;
	int ino;

	if (pathToUnlink != NULL)
	{
		if (pathToUnlink[0] == '/')
		{
		  fd = root->dev;
		  if(isDebug) printf("Unlink from root->dev: fd = %d\n", fd);
		  isRootPath = 1;
		}
		else
		{
		  fd = running->cwd->dev;
		  if(isDebug) printf("Unlink from running->cwd->dev: fd = %d\n", fd);
		  isRootPath = 0;
		}

		// Set dirname and basename globals given pathname
		setDirnameBasename(pathToUnlink);
		// Set the parent and child equal to the new dirname/basename globals
		parent = dirname_value;
		child = basename_value;
		if (isDebug) printf("RAW: Parent: %s\nChild: %s\n", parent, child);
		// If we are a root path with no '/' in parent, fix parent
		if(strcmp(parent, "") == 0)
		{
			if(isRootPath == 1)
			{
				// Parent is null, but we are a root path, set parent == root
				parent = "/";
			}
		}	
		if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);

		// If the child is null, we cannot create this directory
		if(strcmp(child, "") == 0 || child == NULL)
		{
			printf("Cannot Create Empty Directory!\n");
			return;
		}

		// Get the inode number of the parent MINODE
		if (isDebug) printf("Setting parentInode\n");

		// Flip this switch so that getino returns the parent
		FindParent = 1;
		parentInode = getino(&root->dev, parent);
		FindParent = 0;

		childInode = getino(&root->dev, child);
		// If we are not a root parent, and our inode is 0, set the parent node to the cwd
		if(strcmp(parent, "") == 0)
		{		
			if (isRootPath == 0)
			{
				// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
				parentInode = running->cwd->ino;
			}
		}
		// Check if parent inode does not exist
		if (parentInode == 0)
		{		
			printf("The Given Path Contains a non-existant directory\n");
			return;
		}
		
		// Get the In_MEMORY minode of parent:
		if (isDebug) printf("Setting parentMinodePtr\n");
		pmip = iget(root->dev, parentInode);
		mip = iget(root->dev, childInode);
	}
	else
	{
		printf("Pathname cannot be null!\n");
		return;
	}

//	if (S_ISDIR(mip->INODE.i_mode))
//	{
//		printf("Cannot unlink a directory, returning to main\n");
//		return;
//	}
	mip->INODE.i_links_count--;
	if (mip->INODE.i_links_count == 0)
	{
		truncate(mip->dev,mip);//this deallocates all datablocks of an inode, in a similar way you would print them
		idealloc(mip->dev,mip->ino);
	}

	my_rm_dir_Helper(pmip, basename_value);//same as rmdir, just delete that from the path
}

void sym_link(char *oldPath, char *newPath)
{
	char *parent, *child;
  	int parentInode, childInode;
  	int isRootPath = 0;
  	MINODE *Omip, *Nmip;

	char buf[BLOCK_SIZE];
	int i;
	
	if (isDebug) printf("\nNUMBER OF COMMANDS: %d\n\n", numberOfCommands);
	if(numberOfCommands < 3)
	{
		printf("Not enough Parameters entered for symlink");
		return;
	}
	
	if(oldPath != NULL)
	{
		if(oldPath[0] == '/')
		{
			fd = root->dev;
		  	if(isDebug) printf("OLD Symlink from root->dev: fd = %d\n", fd);
		  	isRootPath = 1;
		}
		else
		{
		  	fd = running->cwd->dev;
		  	if(isDebug) printf("OLD Symlink from running->cwd->dev: fd = %d\n", fd);
		  	isRootPath = 0;
		}
		// Set dirname and basename globals given pathname
		setDirnameBasename(oldPath);

		// Set the parent and child equal to the new dirname/basename globals
		parent = dirname_value;
		child = basename_value;
		if (isDebug) printf("RAW: Parent: %s\nChild: %s\n", parent, child);
		if(strcmp(parent, "") == 0)
		{
			if(isRootPath == 1)
			{
				// Parent is null, but we are a root path, set parent == root
				parent = "/";
			}
		}	
		if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);
		// If the child is null, we cannot create this link
		if(strcmp(child, "") == 0 || child == NULL)
		{
			printf("Cannot Create Empty Link!\n");
			return;
		}
		// Get the inode number of the parent MINODE
		if (isDebug) printf("Setting parentInode\n");
		FindParent = 1;
		parentInode = getino(&root->dev, parent);
		FindParent = 0;
		// If the parent is empty, we know that it needs to be the cwd
		if(strcmp(parent, "") == 0)
		{		
			if (isRootPath == 0)
			{
				// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
				parentInode = running->cwd->ino;
			}
		}
		// Check if parent inode does not exist
		if (parentInode == 0)
		{		
			printf("The Given Path Contains a non-existant directory\n");
			return;
		}
		
		// Get the In_MEMORY minode of parent:
		if (isDebug) printf("Setting parentMinodePtr\n");
		Omip = iget(root->dev, parentInode);
		childInode = search(Omip, child);

		// Check if child directory does not exist
		if (childInode == 0)
		{		
			printf("The Given Target does not exist\n");
			return;
		}	
		
		// set child Minodeptr
		if (isDebug) printf("Setting childMinodePtr\n");
		Omip = iget(root->dev, childInode);
	}

	// If the child exists, then we can continue, else, exit
	if (!childInode)
	{
		printf("File to link not found, returning\n");
		return;
	}

	if (isDebug) printf("Setting pathNum to 2\n");
	pathNum = 2;
	// This is too make get inode only find the parent dir to create the new file in
	//strncpy(command[0],"creat",strlen("creat"));
	FindParent = 1;
	if (isDebug) printf("Creating new file %s\n", newPath);
	my_creat(newPath);//create the file that will link to OldName
	FindParent = 0;
	// This is too reset the command[0] to be accurate
	//strncpy(command[0],"symlink",strlen("symlink"));
	pathNum = 1;



	// Get the inode of newPath into memory
	if (isDebug) printf("Getting INODE of %s into memory\n", newPath);
	if(newPath != NULL)
	{
		if (newPath[0] == '/')
		{
		  	fd = root->dev;
		  	if(isDebug) printf("Symlink from root->dev: fd = %d\n", fd);
		  	isRootPath = 1;
		}
		else
		{
		  	fd = running->cwd->dev;
		  	if(isDebug) printf("Symlink from running->cwd->dev: fd = %d\n", fd);
		  	isRootPath = 0;
		}

		// Set dirname and basename globals given pathname
		setDirnameBasename2(newPath);

		// Set the parent and child equal to the new dirname/basename globals
		parent = dirname_value;
		child = basename_value;
		if (isDebug) printf("RAW: Parent: %s\nChild: %s\n", parent, child);

		if(strcmp(parent, "") == 0)
		{
			if(isRootPath == 1)
			{
				// Parent is null, but we are a root path, set parent == root
				parent = "/";
			}
		}	

		if (isDebug) printf("FIXED: Parent: %s\nChild: %s\n", parent, child);
		// If the child is null, we cannot create this link
		if(strcmp(child, "") == 0 || child == NULL)
		{
			printf("Cannot Create Link in Non-existant file!\n");
			return;
		}

		// Get the inode number of the parent MINODE
		if (isDebug) printf("Setting parentInode\n");
		pathNum = 2;
		parentInode = getino(&root->dev, parent);
		pathNum = 1;

		if(strcmp(parent, "") == 0)
		{		
			if (isRootPath == 0)
			{
				// Parent is bad, we are not a root path, but we were given no dirname, set parent to cwd
				parentInode = running->cwd->ino;
			}
		}

		// Check if parent inode does not exist
		if (parentInode == 0)
		{		
			printf("The Given Path Contains a non-existant directory\n");
			return;
		}
		
		// Get the In_MEMORY minode of parent:
		if (isDebug) printf("Setting parentMinodePtr\n");
		Nmip = iget(root->dev, parentInode);

		childInode = search(Nmip, child);
		Nmip = iget(root->dev, childInode);
	}

	// We now have a handle to the file we just created
	if (isDebug) printf("Nmip->ino = %d\n", Nmip->ino);
	Nmip->INODE.i_mode = SYM_LINK;
	int bno = balloc(fd);
	Nmip->INODE.i_block[0] = bno;
	memset(buf, 0, BLKSIZE);
	dp = (DIR *)buf;
	
	dp->name_len = (strlen(oldPath));
	dp->rec_len = BLOCK_SIZE;
	strncpy(dp->name, oldPath, dp->name_len); ////////////////////////////////////////////////////////////////////////SHOULD THIS BE THE LOCAL PATH OR THE ABSOLUTE PATH?
	put_block(fd, Nmip->INODE.i_block[0], buf);
	//write the string oldName into the i_block[], which has room for 60 chars
	//this I have no idea how to do so we will have to tackle it together
	if (isDebug) printf("Putting away Nmip\n");
	iput(Nmip);
}


void read_link(char *linkedPath)
{
	if(isDebug) printf("Read_Link linkedPath = %s\n", linkedPath);

	MINODE *mip;
	char buf[BLOCK_SIZE];
	if(isDebug) printf("get ino\n");
	int ino = 0;

	if(readLinkFromLs > 0) // Calling from ls
	{
		ino = readLinkFromLs;
	}
	else // Calling this manually
	{
		ino = getino(&fd, linkedPath);
	}
	if(isDebug) printf("got ino = %d\n", ino);
	mip = iget(fd, ino);

	if(isDebug) printf("Read_Link before symlink check\n");

	if (mip->INODE.i_mode != SYM_LINK)
	{
		printf("File is not a symbolic link, returning.\n");
		return;
	}


	if(isDebug) printf("Read_Link Before get block\n");

	get_block(fd, mip->INODE.i_block[0], buf);

	if(isDebug) printf("Read_Link after get block\n");

	DIR *dp = (DIR *)buf;

	printf("SymLink: %s->%s\n", linkedPath, dp->name);

	return;

}


void my_touch(char *file)
{
	if (getino(&fd, file) == 0)
	{
		// This is too make get inode only find the parent dir to create the new file in
		strncpy(command[0],"creat",strlen("creat"));
		my_creat(file);
	}
	else
	{
		int ino;
		MINODE *mip;
		ino = getino(&fd, file);
		mip = iget(fd, ino);
		mip->INODE.i_mtime = time(NULL);
		mip->dirty = 1;
		iput(mip);
	}
}

void my_chmod(char *filename, int perm)
{
	int ino = 0, dev;
	MINODE *mip;

	pathNum = 2;
	ino = getino(&fd, filename);
	pathNum = 1;
	dev = root->dev;

	if(ino == 0)
	{
		printf("The given file does not exist\n");
		return;
	}

	mip = iget(dev, ino);

	// Handle the actual permission changes
	if((mip->INODE.i_mode & 0100000) == 0100000)
	{
		if(isDebug) printf("CHMOD a file: %04o\n", strtol(perm, NULL, 8));
		mip->INODE.i_mode = 0100000 + strtol(perm, NULL, 8);
	}
	else if ((mip->INODE.i_mode & 0040000) == 0040000)
	{
		if(isDebug) printf("CHMOD a dir: %04o\n",strtol(perm, NULL, 8));
		mip->INODE.i_mode = 0040000 + strtol(perm, NULL, 8);
	}
	else
	{
		if(isDebug) printf("CHMOD a link: %04o\n", strtol(perm, NULL, 8));
		mip->INODE.i_mode = 0120000 + strtol(perm, NULL, 8);
	}

	// Mark dirty
	mip->dirty = 1;
	mip->INODE.i_mtime = time(0);
	iput(mip);
}

/*
int open_File(char *fileName, int mode)
{
	MINODE *mip;
	int device, ino, i;

	if (file[0] == '/') device = root->dev;
	else device = running->cwd->dev;

	ino = getino(&device, file);

	mip = iget(device, ino);

	if (mip->INODE.i_mode != FILE_MODE)
	{
		printf("NOT A REGULAR FILE, RETURNING\n");
		return;
	}

	for (i = 0; i < NFD; i++) {
		if (running->fd[i] != 0)
		{
			if (running->fd[i]->minodeptr == mip) {
				if (running->fd[i]->mode > 0) {
					printf("file already opened!\n");
					return -1;
				}
			}
		}
	}

	OFT *newFile = malloc(sizeof(OFT));
	int location = falloc(newFile);

	if (location == -1)
	{
		printf("No available open file descriptors, please close something and try again\n");
		return;
	}

	newFile->mode = mode;
	newFile->refCount = 1;
	newFile->inodeptr = mip;

	switch (mode)
	{
	case(0): newFile->offset = 0;
		break;
	case(1): truncate(device, mip);
		newFile->offset = 0;
		break;
	case(2): newFile->offset = 0;
		break;
	case(3): newFile->offset = mip->INODE.i_size;
		break;
		
	}
}
*/
////////////////////////////////////////////////////////////////////////////////////////// I THINK WE ARE MISSING idealloc and bdealloc (We also need a falloc(later) for oft's)
void debug_flip()
{
	if(isDebug)
	{
		printf("\nNo Longer Debugging");
		isDebug = 0;
	}
	else
	{
		printf("Now Debugging");
		isDebug = 1;
	}
}

void commandTable()
{
  if(strcmp(command[0], "help") == 0)
  {
  	my_help();
  }
  if(strcmp(command[0], "ls") == 0)
  {
    my_ls(command[1]); // Should pass in the entire path, as long as it is arg 2
  }
  else if(strcmp(command[0], "pwd") == 0)
  {
    my_pwd();
  }
  else if(strcmp(command[0], "cd") == 0)
  {
  	my_cd(command[1]); // Should pass in the entire path, as long as it is arg 2
  }
  else if(strcmp(command[0], "mkdir") == 0)
  {
  	my_make_dir(command[1]); // Should pass in the entire path, as long as it is arg 2
  }
  else if(strcmp(command[0], "creat") == 0)
  {
  	my_creat(command[1]); // Should pass in the entire path, as long as it is arg 2
  }
  else if (strcmp(command[0], "link") == 0)
  {
	  my_link(command[1], command[2]);
  }
  else if (strcmp(command[0], "unlink") == 0)
  {
	  my_unlink(command[1]);
  }
  else if (strcmp(command[0], "symlink") == 0)
  {
	  sym_link(command[1], command[2]);
  }
  else if (strcmp(command[0], "readlink") == 0)
  {
	  read_link(command[1]);
  }
  else if (strcmp(command[0], "rmdir") == 0)
  {
	my_rm_dir(command[1]); // Should pass in the entire path, as long as it is arg 2
  }
  else if (strcmp(command[0], "touch") == 0)
  {
	  my_touch(command[1]);
  }
  else if (strcmp(command[0], "chmod") == 0)
  {
	  my_chmod(command[2],command[1]);
  }
  else if (strcmp(command[0], "debug") == 0)
  {
  	  debug_flip();
  }
  else if (strcmp(command[0], "rm") == 0)
  {
  	  my_rm(command[1]);
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

  // Print help menu
  my_help();

  // Get commands from stdin
  //MAGIC LOOP    
  while(1)
  {
    // Print prompt for user command entry
    printf("\nUID-%d@J&J EXT2FS:~", running->uid);
    recursive_pwd(running->cwd, 0);
    printf("$ ");
    // Read in and tokenize Command
    numberOfCommands = tokenizeCommmand();
    if (strcmp(command[0], "quit") == 0)
    {
      //User entered Quit, we should probably exit
      for (i = 0; i < NMINODES; i++)
      {
      	if(minode[i].ino != 0)
      	{
      		if(isDebug)printf("Putting away minode[%d]\n", i);
      		// If we have this inode loaded in memory, put it away!
      		iput(&minode[i]);
      	}
      }

 	  char buf[BLKSIZE];
  	  get_block(fd, 1, buf);
  	  sp = (SUPER *)buf;

      printf("EXT2 FS: s_magic = %x\n", sp->s_magic);
      break;
    }
    else
    {
      if(isDebug)
      {
        if (isDebug) printf("Debug Echo: ");
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