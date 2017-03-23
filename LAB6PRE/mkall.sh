#!/bin/sh
echo "gcc -m32 -o super super.c"
gcc -m32 -o super super.c

echo "gcc -m32 -o inode inode.c"
gcc -m32 -o inode inode.c

echo "gcc -m32 -o imap imap.c"
gcc -m32 -o imap imap.c

echo "gcc -m32 -o ialloc ialloc.c"
gcc -m32 -o ialloc ialloc.c

echo "gcc -m32 -o gd gd.c"
gcc -m32 -o gd gd.c

echo "gcc -m32 -o dir dir.c"
gcc -m32 -o dir dir.c

echo "gcc -m32 -o bmap bmap.c"
gcc -m32 -o bmap bmap.c

echo "gcc -m32 -o balloc balloc.c"
gcc -m32 -o balloc balloc.c
