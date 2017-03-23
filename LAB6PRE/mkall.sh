#!/bin/sh
for i in *.c
do
  echo "gcc -m32 $i -o ${i%.c}.out"
  gcc -m32 -w $i -o ${i%.c}.out
done
