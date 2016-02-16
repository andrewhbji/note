#!/bin/bash
# Bash脚本标准起始行。
# Cleanup, version 2
# 使用root权限运行。

LOG_DIR=/var/log
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
	lines=$1
else
	lines=$LINES    # 使用缺省设置
fi

#  Stephane Chazelas建议使用下面的方法检查命令行参数,
#+ 但是这已经超出了这个阶段教程的范围。
#
#    E_WRONGARGS=85  # Non-numerical argument (bad argument format).
#    case "$1" in
#    ""      ) lines=50;;
#    *[!0-9]*) echo "Usage: `baseman $0` lines-to-cleanup";
#     exit $E_WRONGARGS;;
#    *       ) lines=$1;;
#    esac
#
#* 在第十一章“循环与分支”中会对此作详细的阐述。

# 变量比硬编码(hard-coded)要更合适一些
cd $LOG_DIR

if [ `pwd` != "$LOG_DIR" ]    # Not in /var/log?
# or   if [ "$PWD" != "$LOG_DIR" ]
then
	echo "Can't change to $LOG_DIR"
	exit $E_XCD
fi  # 在清理日志之前,重复确认是否在正确的工作目录下。

# 更高效的方法是:
#
# cd /var/log || {
#   echo "Cannot change to necessary directory." > &2
#   exit $E_XCD;
# }

tail -n $lines messages > msg.temp # 保存messages的最后一部分
mv msg.temp messages         # 替换原来的messages达到清理的目的

#  cat /dev/null > messages
#* 我们不再需要使用这个方法了,使用上面的方法更安全

cat /dev/null > wtmp  #  ': > wtmp' 与 '> wtmp' 都有同样的效果

echo "Log files cleaned up."
#  注意在/var/log目录下其他的日志文件将不会被这个脚本清除
exit 0
#  返回0表示脚本运行成功
