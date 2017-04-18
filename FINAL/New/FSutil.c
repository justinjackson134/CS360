/* util.c file: This file contains utility functions of the file system. The most important
utility functions are getino(), iget() and iput() */

/* u32 getino(int *dev, char *pathname): getino() returns the inode number of a pathname. 
While traversing a pathname the device number may change if the pathname crosses mounting 
point(s). The parameter dev is used to record the final device number. Thus, getino() 
essentially returns the (dev, ino) of a pathname. The function uses token() to break 
up pathname into component strings. Then it calls search() to search for the component 
strings in successive directory minodes. */




/* MINODE *iget(in dev, u32 ino): This function returns a pointer to the inmemory INODE
of (dev, ino). The returned minode is unique, i.e. only one copy of the INODE exists in 
memory. In addition, the minode is locked for exclusive use until it is either released 
or unlocked. */



/* iput(MINODE *mip): This function releases and unlocks a minode pointed by mip. If the
process is the last one to use the minode (refCount = 0), the INODE is written back to
disk if it is dirty (modified). (3).4. Use of getino()/iget()/iput(): In a file system,
almost every operation begins with a pathname, e.g. mkdir pathname, cat pathname, etc.
Whenever a pathname is needed, its inode must be loaded into memory for reference. */
