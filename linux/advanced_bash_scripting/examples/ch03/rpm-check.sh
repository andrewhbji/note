#!/bin/bash
# rpm-check.sh
# 查询一个rpm文件的文件描述、包含文件列表,以及是否可以被安装。
# 将输出保存至文件。
#
# 这个脚本使用代码块来描述。
SUCCESS=0
E_NOARGS=65
if [ -z "$1" ]
then
  echo "Usage: `basename $0` rpm-file"
  exit $E_NOARGS
fi
{ # 代码块起始
  echo
  echo "Archive Description:"
  rpm -qpi $1       # 查询文件描述。
  echo
  echo "Archive Listing:"
  rpm -qpl $1       # 查询文件列表。
  echo
  rpm -i --test $1  # 查询是否可以被安装。
  if [ "$?" -eq $SUCCESS ]
  then
    echo "$1 can be installed."
  else
      echo "$1 cannot be installed."
  fi
  echo              # 代码块结束。
} > "$1.test"       # 输出重定向至文件。
echo "Results of rpm test in file $1.test"
# 查看 rpm 的帮助获得解释。
exit 0
