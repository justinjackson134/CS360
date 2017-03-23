#!/bin/sh
gcc -m32 -o super super.c
gcc -m32 -o inode inode.c
gcc -m32 -o imap imap.c
gcc -m32 -o ialloc ialloc.c
gcc -m32 -o gd gd.c
gcc -m32 -o dir dir.c
gcc -m32 -o bmap.c
gcc -m32 -o balloc.c
