#!/bin/bash
# Advanced-Bash-Scripting-Guide 第二章第二节练习2
# 调用脚本是会显示当前日期和时间，所有已登陆用户，系统正常运行时间，并将这些信息保存到一个文件中
# ex01, version 2
# 使用root权限运行。

LOC_DIR=/tmp
ROOT_UID=0      # UID为0的用户才拥有root权限。
LINES=50       # 缺省需要保存messages文件的行数。
E_XCD=86       # 无法切换工作目录的错误码。
E_NOTROOT=87     # 非root权限用户运行的错误码。

# 使用root权限运行。
if [ "$UID" -ne "$ROOT_UID" ]
then
 echo "Must be root to run this script."
 exit $E_NOTROOT
fi

if [ -n "$1" ]    # 测试命令行参数(保存的行数)是否为空
then
 dir=$1
else
 dir=$LOC_DIR    # 使用缺省设置
fi

# 变量比硬编码(hard-coded)要更合适一些
cd $dir

if [ `pwd` != "$dir" ]
# or   if [ "$PWD" != "$LOC_DIR" ]
then
 echo "Can't change to $dir"
 exit $E_XCD
fi

# 更高效的方法是:
#
# cd $LOC_DIR || {
#   echo "Cannot change to necessary directory." > &2
#   exit $E_XCD;
# }

uptime=$(w |sed '2,$d' |awk '{print $3}' |sed 's/,//g') #系统运行时间
current=$(date "+%Y年 %m月 %d日 %H:%M:%S")
users=$(who |awk '{print $1}')

echo $uptime
echo $current
echo $users

$(echo $uptime >> temp.txt)
$(echo $current >> temp.txt)
$(echo $users >> temp.txt)

echo "Exit."
#  注意在/var/log目录下其他的日志文件将不会被这个脚本清除
exit 0
#  返回0表示脚本运行成功
