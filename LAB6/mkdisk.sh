#! /bin/sh
# mkdisk
    touch mydisk
    sudo mkfs mydisk 1440
    sudo mount -o loop mydisk /mnt
    (cd /mnt; mkdir a b c d; touch f1 f2 f3)
    ls -l /mnt
    sudo umount /mnt
