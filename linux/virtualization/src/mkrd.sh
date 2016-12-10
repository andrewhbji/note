#!/bin/bash

rm -f rootfs.img.gz
cd rootfs
find . | cpio -o --format=newc | gzip > ../rootfs.img.gz
