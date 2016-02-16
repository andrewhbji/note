#!/bin/bash
# 读取文件 /etc/fstab。
File=/etc/fstab
{
read line1
read line2
} < $File
echo "First line in $File is:"
echo "$line1"
echo
echo "Second line in $File is:"
echo "$line2"
exit 0
# 那么,你知道如何解析每一行不同的字段么?
# 提示:使用 hint 或者
# Hans-Joerg Diers 建议使用Bash的内建命令 set。
