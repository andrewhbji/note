[TOC]
# 子shell
## ( command1; command2; command3; ... )
```sh
(
# Inside parentheses, and therefore a subshell . . .
while [ 1 ]   # Endless loop.
do
  echo "Subshell running . . ."
done
)
```
## 子shell的变量作用域
- 子shell内定义的变量在子shell外无法访问
- 子shell可以访问外部定义的变量
- 子shell对外部定义变量进行操作，其范围只在子shell内有效
### 串行执行子shell
```sh
(cat list1 list2 list3 | sort | uniq > list123) &
(cat list4 list5 list6 | sort | uniq > list456) &
# Merges and sorts both sets of lists simultaneously.
# Running in background ensures parallel execution.
#
# Same effect as
#   cat list1 list2 list3 | sort | uniq > list123 &
#   cat list4 list5 list6 | sort | uniq > list456 &

wait   # Don't execute the next command until subshells finish.

diff list123 list456
```
