#!/bin/bash
#	Bash脚本标准起始行。
#	Cleanup,	version	2
#	使用root权限运行。
#	在这里插入代码用来打印错误信息,并且在未使用root权限时退出。

LOG_DIR=/var/log
#	变量比硬编码(hard-coded)要更合适一些
cd	$LOG_DIR

cat	/dev/null	>	messages
cat	/dev/null	>	wtmp

echo	"Logs	cleaned	up."
exit	#	这是正确终止脚本的方式。
					#	不带参数的exit返回的是上一条命令的结果。
