[TOC]
#脚本的结构
##脚本起始行 #！
指向脚本解释器的路径，Linux下默认为/bin/bash，UNIX下默认为/bin/sh
/bin/bash和/bin/sh不完全一样，所以兼容/bin/sh的脚本需要遵循POSIX sh标准
##exit
终止脚本
#调用一个脚本
##授权 chmod
##调用
###直接运行
/usr/local/bin下直接敲脚本名，其它路径下需要加入路径
###使用sh调用
使用sh调用时，Bash脚本会禁用掉bash特性
###使用source调用
