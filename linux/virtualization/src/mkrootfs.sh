#!/bin/bash

sudo rm -rf rootfs
sudo rm -f a9rootfs.ext3

mkdir rootfs
cp busybox/_install/*  rootfs/ -raf

mkdir -p rootfs/proc/
mkdir -p rootfs/sys/
mkdir -p rootfs/tmp/
mkdir -p rootfs/root/
mkdir -p rootfs/var/
mkdir -p rootfs/mnt/

cp etc rootfs/ -arf

cp -arf $GCCLIB_PATH rootfs/

rm rootfs/lib/*.a
${CROSS_COMPILE}strip rootfs/lib/*

mkdir -p rootfs/dev/
sudo mknod rootfs/dev/tty1 c 4 1
sudo mknod rootfs/dev/tty2 c 4 2
sudo mknod rootfs/dev/tty3 c 4 3
sudo mknod rootfs/dev/tty4 c 4 4
sudo mknod rootfs/dev/console c 5 1
sudo mknod rootfs/dev/null c 1 3
