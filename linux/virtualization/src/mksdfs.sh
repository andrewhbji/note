#!/bin/bash

sudo rm -rf tmpfs
sudo rm -f a9rootfs.ext3

dd if=/dev/zero of=a9rootfs.ext3 bs=1M count=32
mkfs.ext3 a9rootfs.ext3

mkdir -p tmpfs
sudo mount -t ext3 a9rootfs.ext3 tmpfs/ -o loop
sudo cp -r rootfs/*  tmpfs/
sudo umount tmpfs
