[TOC]
# 内部命令与内建(built-in)
builtin是指命令包含在Bash工具集中。
Bash使用这样的方式设计内部命令主要考虑到执行效率：
1. builtin相较外部命令而言执行更快
2. 特定的内部命令需要直接方位shell内部

- 通常情况下，内部命令在运行的时候是不会fork子进程
- 内部命令通常与系统命令重名，但Bash内部 重新实现了这些命令
- 关键字不是内部命令，而是比较大的命令结构的一部分
## 常见的内部命令
### I/O
#### echo
打印表达式或者变量到stdout，默认会新起一行打印
- -e 指出打印转义字符
- -n 不新起一行打印
- echo可以将stdout通过管道传递到其他一系列命令中
```sh
if echo "$VAR" | grep -q txt   # if [[ $VAR = *txt* ]]
then
  echo "$VAR contains the substring sequence \"txt\""
fi
```
- echo可以将stdout进行命令替换
```sh
a=`echo "HELLO" | tr A-Z a-z`
```
#### printf
格式化输出，语法类似 C 语言的printf()函数
```sh
printf
format-string ... parameter ...
```
#### read
从 stdin 中读取一个变量的值
- -a 选项可以 read 数组变量
- read 未指定参数，输入将分配给默认变量 $REPLY
- -r 选项会让 "\\" 转义，从而禁用多行输入功能
```sh
read var1     # The "\" suppresses the newline, when reading $var1.
              #     first line \
              #     second line

echo "var1 = $var1"
read -r var2  # The -r option causes the "\" to be read literally.
              #     first line \

echo "var2 = $var2"
#     var2 = first line \

# 第一个 <ENTER> 就会结束var2变量的录入.
```
- -n 检测方向键
```sh
uparrow=$'\x1b[A'
downarrow=$'\x1b[B'
leftarrow=$'\x1b[D'
rightarrow=$'\x1b[C'

read -s -n3 -p "Hit an arrow key: " x

case "$x" in
$uparrow)
   echo "You pressed up-arrow"
   ;;
$downarrow)
   echo "You pressed down-arrow"
   ;;
$leftarrow)
   echo "You pressed left-arrow"
   ;;
$rightarrow)
   echo "You pressed right-arrow"
   ;;
esac
```
- -t 指定输入超时时间，超时将break
```sh
TIMELIMIT=4         # 4 seconds

read -t $TIMELIMIT variable <&1
#                           ^^^
#  In this instance, "<&1" is needed for Bash 1.x and 2.x,
#  but unnecessary for Bash 3+.
```
- read 命令也可以从重定向的文件中"读取"变量的值
  - 如果文件超过一行，那么只有一行被分配到这个变量中
  ```sh
  read var1 </etc/passwd
  echo "var1 = $var1" # var1 = root:x:0:0:root:/root:/bin/bash
  ```
  - 如果 read 命令的参数超过一个，那么每个变量都会从文件的第一行取得一个分配的字符串作为变量的值，这些字符串都是以空白符$IFS进行分割的，最后一个变量将会读取第一行剩下的部分，并且如果需要赋值的变量个数比文件第一行分割的字符串个数多的话，剩余的变量将会被赋空值
  ```sh
  read var2 var3 </etc/passwd #默认使用换行符分割
  echo "var2 = $var2   var3 = $var3" # var2 = root:x:0:0:root:/root:/bin/bash   var3 =
  # var2读取了第一行整行，var3为空
  IFS=: # 使用:将passwd文件分割
  read var2 var3 </etc/passwd
  echo "var2 = $var2   var3 = $var3" # var2 = root   var3 = x:0:0:root:/root:/bin/bash
  # var2 读取了:分割的第一个字串，var3读取了剩下的字串
  ```
  - 可以使用循环 read 的方式处理整个数据文件
  ```sh
  # 将passwd文件的每一行分割为6段，然后输出第一段和第五段
  while IFS=: read name passwd uid gid fullname ignore
  do
    echo "$name ($fullname)"
  done </etc/passwd   # I/O redirection.
  ```
### 文件系统
#### cd
修改当前工作目录
- -P(physical) 选项对于 cd 命令的意义是忽略符号链接
- cd - 把工作目录修改至之前的工作目录 $OLDPWD
#### pwd
打印当前工作目录，即当前目录栈最顶层的目录
#### pushd
pushd dir-name把dir-name路径压入目录栈
#### popd
将目录栈最顶部的目录出栈
#### dirs
列出所有目录栈的内容
### 变量
#### let
```sh
let <表达式>
```
执行变量的算术操作
#### eval
```sh
eval arg1 [arg2] ... [argN]
```
将表达式中的参数，或者表达式列表组合起来，然后执行他们
- 执行命令
```sh
y=`eval ls -l`  #  Similar to y=`ls -l`
echo $y         #+ but linefeeds removed because "echoed" variable is unquoted.
echo
echo "$y"       #  Linefeeds preserved when variable is quoted.
y=`eval df`     #  Similar to y=`df`
echo $y         #+ but linefeeds removed.
eval "`seq 3 | sed -e 's/.*/echo var&=ABCDEFGHIJ/'`" # 生成三个序列，然后使用sed将序列进行格式化
# var1=ABCDEFGHIJ
# var2=ABCDEFGHIJ
# var3=ABCDEFGHIJ
```
- 处理变量
```sh
a='$b'
b='$c'
c=d
echo $a             # $b
                    # First level.
eval echo $a        # $c
                    # Second level.
eval eval echo $a   # d
                    # Third level.

version=3.4     #  Can we split the version into major and minor
                #+ part in one command?
eval major=${version/./;minor=}     #  Replaces '.' in version by ';minor='
                                    #  The substitution yields '3; minor=4'
                                    #+ so eval does minor=4, major=3
```
- 处理数组
```sh
eval array_member=\${arr[element_number]} #element_number=0,1,2...
```
- 处理命令行参数
```sh
eval echo \$$param #param = 1,2,3...
```
#### set
set命令用来修改内部脚本变量的值.
- 触发 选项标志位 来帮助决定脚本的行为
- 以一个命令的结果(set 'command')来重新设置脚本的位置参数
```sh
echo "Command-line argument #1 = $1"
echo "Command-line argument #2 = $2"
echo "Command-line argument #3 = $3"
set a b c
echo "Field #1 of 'a b c' = $1" # a
echo "Field #2 of 'a b c' = $2" # b
echo "Field #3 of 'a b c' = $3" # c

variable="one two three four five"
set -- $variable
echo "first parameter = $first_param" # one
echo "second parameter = $second_param" # two
set --  # 如果没指定变量,那么将会unset所有的位置参数
echo "first parameter = $first_param" # null value
"second parameter = $second_param" # null value
```
#### unset
将一个变量设置为null
```sh
var=hello
unset var
echo $var # null value
```
#### export
export 命令将会使得被 export 的变量在所运行脚本(或shell)的所有子进程中都可用。
```sh
export column，number # 这两个变量导出后，就可以在后面的awk进程中使用了
awkscript='{ total += $ENVIRON["column_number"] }END { print total }'
awk "$awkscript" "$filename"
```
- 可以在一个操作中同时进行赋值和export变量, 比如: export var1=xxx.
- 子shell中export的变量不能在父shell中使用
#### declare,typeset
declare和typeset命令被用来指定或限制变量的属性
#### readonly
与declare -r作用相同, 设置变量的只读属性，可以理解为声明常量
#### getopts
用来处理命令行参数
- getopts 将选项(如 -a、-abc)作为参数在脚本中处理
- getopts 允许传递、连接、分配多个选项到脚本，如scriptname -abc -e
- getopts 不处理不带 "-(减号)" 的选项
- getopts 结构隐含两个变量，$OPTIND 是参数指针( 选项索引) 和 $OPTARG ( 选项参数)(可选的可以在选项后边附加一个参数，如-rXXX 或 -r XXX)
- getopts 结构通常组成一组放在一个while循环中，循环每次处理一个选项和参数，然后$OPTIND自增，再进行下一次处理
```sh
while getopts ":abcde:fg" Option
# 开始的声明.
# a, b, c, d, e, f, 和 g 被认为是选项(标志).
# 'e' 选项后边的 : 提示这个选项需要带一个参数.
# 如果选项'e'不带参数进行调用的话, 会产生一个错误信息.
# 这个开头的 : 就是用来屏蔽掉这个错误信息的,因为我们一般都会有默认处理, 所以并不需要这个错误信息.
do
  case $Option in
    a ) ;; # 对选项'a'作些操作.
    b ) ;; # 对选项'b'作些操作.
    ...
    e) ;;  # 对选项'e'作些操作, 同时处理一下$OPTARG,
        # 这个变量里边将保存传递给选项"e"的参数.
    ...
    g ) ;; # 对选项'g'作些操作.
  esac
done
shift $(($OPTIND - 1))
# 将参数指针向下移动.
```
### 脚本行为
#### source,.(点 命令)
- 当在命令行中调用的时候, 这个命令将会执行一个脚本
- 在脚本中调用的时候, source file-name 将会加载 file-name 文件(与 C 语言中的 #include 指令效果相同),最终效果就像是在source的位置上插入了file-name的内容
- 如果source进来的文件本身就一个可执行脚本的话, 那么它将运行起来, 然后将控制权交还给调
用它的脚本
#### exit
无条件的停止一个脚本的运行
- 如果不带参数调用 exit ，退出状态码是脚本中最后一个命令的退出状态码. 等价于exit $?
#### exec
使用exec 执行外部命令时，shell 不会 fork 一个子进程执行该命令
#### shopt
修改shell选项
- -s 设置
- -u 取消设置
```sh
shopt -s cdspell
# Allows minor misspelling of directory names with 'cd'

cd /hpme  # Oops! Mistyped '/home'.
pwd       # /home
          # The shell corrected the misspelling.
```
#### caller
将 caller 命令放在函数中，会在 stdout 上打印函数调用者的信息
### 命令
#### true
一个返回成功(就是返回 0)退出码的命令,但是除此之外什么事也不做.
#### false
一个返回失败(非 0)退出码的命令,但是除此之外什么事也不做.
#### type [cmds]
type 命令用于查看 命令 的完整路径，type 的 -a 选项可以鉴别所识别的参数是关键字、内建命令还是同名系统命令
#### hash [cmds]
- 不带参数的 hash 命令用于列出 shell 的 hash表 的内容
- -r 选项会重新设置 shell 的 hash 表
- hash [cmds] 将 命令 cmds 的路径存入 shell 的 hash表
>shell 的 hash表用于记录以调用命令的路径，如果某个命令已经被调用过，那么这个命令的路径将记录在hash表中，而不需要再在$PATH中重新搜索这个命令

```sh
$ ls
$ which
$ wget
$ hash
命中	命令
   1	/usr/bin/which
   1	/usr/bin/wget
   1	/bin/ls

```
#### bind
bind 内建命令用来显示或修改 readline 的键绑定.
#### help [cmds]
显示 cmds 的用法摘要
## 15.1 任务控制命令
### jobs
在后台列出所有正在运行的任务,给出任务标识符.
```sh
bash$ sleep 100 &
[1] 1384 # "1"是任务标识符,
# kill 掉任务或进程，可以使用 kill % <任务标识符> 或者使用 kill <进程ID>
bash $ jobs
[1]+  Running                 sleep 100 &
```
### disown
从 shell 的当前任务表中,删除任务.
```sh
$ sleep 100 &
[1] 1384
$ jobs
[1]+  Running                 sleep 100 &
$ disown
$ jobs # sleep 100 & 任务已经被删除
```
### fg,bg
- fg 可以把一个在后台运行的任务放在前台运行
- bg 会重启一个挂起的任务
### wait
```sh
wait [% 人物标识符 或 $ PPID]
```
暂停脚本的运行,直到后台运行的所有任务都结束为止,或者直到指定任务标识符或进程号为选项的任务结束为止.
> wait 命令来防止在后台任务没完成(这会产生一个孤儿进程)之前退出脚本

### suspend
这个命令的效果与 Control-Z 很相像,但是它挂起的是这个 shell(这个 shell 的父进程应该在合适的时候重新恢复它).
### logout
退出一个登陆的 shell,也可以指定一个退出码.
### times
给出执行命令所占的时间,使用如下形式输出:0m0.020s 0m0.020s
### kill
通过发送一个适当的结束信号,来强制结束一个进程
### command
command 命令会禁用别名和函数的查找.它只查找内部命令以及搜索路径中找到的脚本或可执行程序
### builtin
在"builtin"后边的命令将只调用内建命令.暂时的禁用同名的函数或者是同名的扩展命令.
### enable
这个命令或者禁用内建命令或者恢复内建命令.如: enable -n kill 将禁用 kill 内建命令,所以当我们调用 kill 时,使用的将是/bin/kill 外部命令.
- -a 选项将会恢复相应的内建命令,如果不带参数的话,将会恢复所有的内建命令.
- -f filename 选项 将会从适当的编译过的目标文件中以共享库(DLL)的形式来加载一个内建命令.
### autoload
这是从 ksh 的 autoloader 命令移植过来的.一个带有"autoload"声明的函数,在它第一次被调用的时候才会被加载. 这样做会节省系统资源.

### 表 15-1 任务标识符
| 符号 | 含义 |
|------|------|
| %N | 作业号[N] |
| %S| 以字符串 S 开头的被(命令行)调用的作业 |
| %?S| 包含字符串 S 的被(命令行)调用的作业|
| %% | 当前作业(前台最后结束的作业,或后台最后启动的作业) |
| %+ | 当前作业(前台最后结束的作业,或后台最后启动的作业) |
| %- | 最后的作业 |
| $! | 最后的后台进程 |
