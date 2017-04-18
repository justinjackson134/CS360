/*  mount_root.c file This file contains the mount_root() function, which is called
during system initialization to mount the root file system. It reads the superblock
of the root device to verify the device is a valid EXT2 file system. It loads the 
root INODE (ino = 2) into a minode and sets the root pointer to the root minode. 
Then it unlocks the root minode to allow all processes to access the root minode. 
A mount table entry is allocated to record the mounted root file system. Some key 
parameters on the root device, such as the starting blocks of the bitmaps and inodes
table, are also recorded in the mount table for quick reference. */