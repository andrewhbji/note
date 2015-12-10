#!/bin/bash
# Program:
#	User input dir name, I find the permission of files.
# History:
#2007/04/01 andrea First release

# 1. 先看看这个目录是否存在啊？
read -p "Please input a directory: " dir
if [ "$dir" == "" -o ! -d "$dir" ]; then
	echo "The $dir is NOT exist in your system."
	exit 1
fi

# 2. 开始测试文件罗～
filelist=$(ls $dir)        # 列出所有在该目录下的文件名称
for filename in $filelist
do
	perm=""
	test -r "$dir/$filename" && perm="$perm readable"
	test -w "$dir/$filename" && perm="$perm writable"
	test -x "$dir/$filename" && perm="$perm executable"
	echo "The file $dir/$filename's permission is $perm "
done
