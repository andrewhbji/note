[TOC]
# 列表结构
用来处理一串连续的命令
## 与列表
```sh
command-1 && command-2 && command-3 && ... command-n
```
- 如果每个命令执行后都返回true(0)的话, 那么命令将会依次执行下去.
- 如果其中的某个命令返回false(非零值)的话, 那么这个命令链就会被打断, 也就是结束执行, (那么第一个返回false的命令, 就是最后一个执行的命令, 其后的命令都不会执行).
## 或列表
```sh
command-1 || command-2 || command-3 || ... command-n
```
- 如果每个命令都返回false, 那么命令链就会执行下去.
- 有一个命令返回true, 命令链就会被打断, 也就是结束执行, (第一个返回true的命令将会是最后一个执行的命令).
> 备注：
> 1. 与列表和或列表的退出状态码由最后一个命令的退出状态所决定.
> 2. 可以灵活的将"与"/"或"列表组合在一起, 但是这么做的话, 会使得逻辑变得很复杂, 并且需要经过仔细的测试.

```sh
false && true || echo false         # false

# Same result as
( false && true ) || echo false     # false
# But NOT
false && ( true || echo false )     # (nothing echoed)

#  Note left-to-right grouping and evaluation of statements.

#  It's usually best to avoid such complexities.

#  Thanks, S.C.
```
