#!/bin/sh
for i in *.c
do
  echo "gcc -m32 $i -o ${i%.c}.out"
done
