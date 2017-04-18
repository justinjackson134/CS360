/* type.h file: This file contains the data structure types of the EXT2 file system, 
 such as superblock, group descriptor, inode and directory entry structures. 
 In addition, it also contains the open file table, mount table, pipes and PROC 
 structures and constants of the MTX kernel. */

/*           THIS IS KC's type.h FILE EXACTLY            */
/* http://eecs.wsu.edu/~cs360/samples/OLD/PROJECT/type.h */

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLOCK_SIZE        1024
#define BLKSIZE           1024
#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

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

// Open File Table
typedef struct Oft{
  int    mode;              // Read | Write
  int    refCount;          // How many procs are using me right now
  struct Minode *inodeptr;  // Pointer to an Minode[]
  long   offset;            // UNKNOWN
} OFT;

// PROC structure
typedef struct Proc{
  int   uid;                // User ID
  int   pid;                // Process ID
  int   gid;                // Group ID
  int   ppid;               // Parent Process ID
  int   status;             // Process Status (FREE, BUSY, KILLED)

  struct Minode *cwd;       // Pointer to a Minode Struct
  OFT   *fd[NFD];           // Open File Table - *File Descriptor[10] (10 defined above as NFD)

  struct Proc *next;        // Next Proc Ptr
  struct Proc *parent;      // Parent Proc Ptr
  struct Proc *child;       // Child Proc Ptr
  struct Proc *sibling;     // Sibling Proc Ptr
} PROC;
      
// In-memory inodes structure
typedef struct Minode{		  // In Memory Inode Struct
  INODE INODE;              // disk inode
  int   dev, ino;           // Store Device # and Inode #
  int   refCount;           // Keep track of how many procs are using this minode
  int   dirty;              // Have there been changes which need to be written to disk?
  int   mounted;            // Is this minode mounted?
  struct Mount *mountptr;   // Pointer to a mount struct
  char     name[128];       // name string of file
} MINODE;

// Mount Table structure
typedef struct Mount{
  int  ninodes;             // Number of INODES
  int  nblocks;             // Number of Blocks
  int  bmap;                // Block map
  int  imap;                // Inode map
  int  iblock;              // Inode block
  int  dev, busy;           // Store device ID and busy status
  struct Minode *mounted_inode; // Ptr to mounted INODE
  char   name[256];         // name of ?
  char   mount_name[64];    // name of ?
} MOUNT;

// function proto types
MINODE *iget();             // iget function proto
OFT    *falloc();           // falloc function proto