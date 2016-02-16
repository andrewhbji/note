[TOC]
# 和变量相关的更多内容
## 9.1 内部变量
### $BASH
这个变量将指向 Bash 的二进制执行文件的位置
### $BASH_ENV
这个环境变量将指向一个 Bash 启动文件,这个启动文件将在调用一个脚本时被读取.
### $BASH_SUBSHELL
这个变量将提醒 subshell 的层次,这是一个在 version3 才被添加到 Bash 中的新特性.
### $BASH_VERSINFO[n]
记录 Bash 安装信息的一个 6 元素的数组
### $BASH_VERSION
安装在系统上的 Bash 的版本号，和$BASH_VERSINFO[n]很相似
### $DIRSTACK
当前路径，和dirs命令输出一致，但是会收到pushd和popd的影响
### EDITOR
脚本调用的默认编辑器,一般是 vi 或者是 emacs
### $EUID
"effective"用户 ID 号
### $FUNCNAME
当前函数的名字，仅在当前函数内有效，在函数外则为NULL
### $GLOBIGNORE
一个文件名的模式匹配列表，如果使用正则表达式(原文为file globbing)匹配到的文件也包含在这个列表中，那么这个文件将从匹配结果中去掉
### $GROUPS
当前用户属于的组
### $HOME
用户的 home 目录
### $HOSTNAME
输出当前系统名字，和hostname命令的输出一样，gethostname()函数用来设置这个内部变量
### $HOSTTYPE
主机类型
### $IFS
内部域分隔符.
这个变量用来决定 Bash 在解释字符串时如何识别域,或者单词边界.默认为空格、tab、换行等
```sh
bash$ echo $IFS | cat -vte
$
bash$ bash -c 'set w x y z; IFS=":-;"; echo "$*"'
w:x:y:z
# $*使用$IFS 中的第一个字符
```
### $IGNOREEOF
忽略 EOF: 告诉 shell 在 log out 之前要忽略多少文件结束符(control-D).
### $LC_COLLATE
用来在文件名扩展和模式匹配校对顺序，常在.bashrc 或/etc/profile 中设置
### $LC_CTYPE
这个内部变量用来控制 globbing 和模式匹配的字符串解释
### LINENO
记录行号，用来调试
### $MACHTYPE
系统硬件类型
### $OLDPWD
之前的工作目录
### $OSTYPE
操作系统类型
### $PATH
指向 Bash 外部命令所在的位置
### $PIPESTATUS
数组变量，每个成员都会保存一个管道命令的退出码，,$PIPESTATUS[0]保存第
一个管道命令的退出码,$PIPESTATUS[1]保存第 2 个,以此类推。$PIPESTATUS 默认为最后一个
### $PPID
父进程ID
### $PS1
主提示符,具体见命令行上的显示
### $PS2
第 2 提示符,当你需要额外的输入的时候将会显示,默认为">"
### $PS3
第 3 提示符,在一个 select 循环中显示
### $PS4
第 4 提示符,当使用-x 选项调用脚本时,这个提示符将出现在每行的输出前边.
默认为"+"
### $PWD
工作目录(你当前所在的目录)，和pwd命令相同
### $REPLY
read 命令如果没有给变量,那么输入将保存在$REPLY 中
### $SECONDS
这个脚本已经运行的时间(单位为秒)
### $SHELLOPTS
这个变量里保存 shell 允许的选项,这个变量是只读的
### $SHLVL
Shell 层次,就是 shell 层叠的层次,如果是命令行那$SHLVL 就是 1,如果命令行执行的脚
本中,$SHLVL 就是 2,以此类推.
### $UID
用户 ID 号.
### 位置参数
#### $0, $1, $2,等等...
位置参数,从命令行传递给脚本
#### $#
命令行或者是位置参数的个数
#### $*
所有的位置参数,被作为一个单词
```sh
for arg in "$*"  # Doesn't work properly if "$*" isn't quoted.
do
  echo "Arg #$index = $arg"
  let "index+=1"
done             # $* sees all arguments as single word.
echo "Entire arg list seen as single word."
# Arg #1 = aaa bbb ccc ddd
# Entire arg list seen as single word.

for arg in $*
do
  echo "Arg #$index = $arg"
  let "index+=1"
done             # Unquoted $* sees arguments as separate words.
echo "Arg list seen as separate words."
# Arg #1 = aaa
# Arg #2 = bbb
# Arg #3 = ccc
# Arg #4 = ddd
# Arg list seen as separate words.
```
#### $@
- 与$\*同义,但是每个参数都是一个独立的""引用字串,这就意味着参数被完整地传递,并没有被解释和扩展.这也意味着,每个参数列表中的每个参数都被当成一个独立的单词
```sh
for arg in "$@"
do
  echo "Arg #$index = $arg"
  let "index+=1"
done             # $@ sees arguments as separate words.
echo "Arg list seen as separate words."
# Arg #1 = aaa
# Arg #2 = bbb
# Arg #3 = ccc
# Arg #4 = ddd
# Arg list seen as separate words.
```
- 但是，在 shift 命令后边,$@将保存命令行中剩余的参数,而$1 被丢掉了，再调用$@,$2将被丢弃，依次类推
```sh
#!/bin/bash
# Invoke with ./scriptname 1 2 3 4 5

echo "$@"    # 1 2 3 4 5
shift
echo "$@"    # 2 3 4 5
shift
echo "$@"    # 3 4 5

# Each "shift" loses parameter $1.
# "$@" then contains the remaining parameters.
```
### 其他特殊参数
#### $-
传递给脚本的 Flag(使用 set 命令)
#### $!
在后台运行的最后的工作的 PID(进程 ID)
可以使用$!进行任务控制
```sh

possibly_hanging_job & { sleep ${TIMEOUT}; eval 'kill -9 $!' &> /dev/null; }
# 强制停止一个品性不良的程序
# Useful, for example, in init scripts.

# Thank you, Sylvain Fourmanoit, for this creative use of the "!" variable.
```
#### $_
保存之前执行的命令的最后一个参数
#### $?
命令,函数或者脚本本身的退出状态
#### $$
脚本自身的进程 ID
## 9.2 指定类型的变量
### declare/typeset 选项
#### -r 只读
这和C语言中的const关键字一样，不能修改
#### -i 整型
被declare声明为整型的变量不能被修改为其他类型
```sh
declare -i number
# The script will treat subsequent occurrences of "number" as an integer.		

number=3
echo "Number = $number"     # Number = 3

number=three
echo "Number = $number"     # Number = 0
# Tries to evaluate the string "three" as an integer.
```
#### -a 数组
#### -f 函数
- 如果使用 declare -f 而不带参数的话,将会列出这个脚本中之前定义的所有函数.
- 如果使用 declare -f function_name 这种形式的话,将只会列出这个函数的名字.
#### -x 导出
declare -x var3 将变量导出供其他脚本使用
declare -x 允许给变量赋值
```sh
declare -x var3=373
```
> 备注： declare会限制变量的作用域
```sh
foo ()
{
FOO="bar"
}

bar ()
{
foo
echo $FOO
}

bar   # Prints bar.
# 但是
foo (){
declare FOO="bar"
}

bar ()
{
foo
echo $FOO
}

bar  # Prints nothing.


# Thank you, Michael Iatrou, for pointing this out.
```

### 9.2.1 declare 的其他作用
用来识别变量、环境变量等等
```sh

bash$ declare | grep HOME
HOME=/home/bozo


bash$ zzy=68
bash$ declare | grep zzy
zzy=68


bash$ Colors=([0]="purple" [1]="reddish-orange" [2]="light green")
bash$ echo ${Colors[@]}
purple reddish-orange light green
bash$ declare | grep Colors
Colors=([0]="purple" [1]="reddish-orange" [2]="light green")
```
## 9.3 $RANDOM 随机数生成器
$RANDOM 是 Bash 的内部函数(并不是常量),这个函数将返回一个范围在 0 - 32767 之间的一个伪随机整数.它不应该被用来产生密匙.
### 随机数公式
```sh
rnumber=$(((RANDOM%(max-min+divisibleBy))/divisibleBy*divisibleBy+min))
# max 随机数最大值
# min 随机数最小值
# divisibleBy 随机数需要被divisibleBy整除
```
>备注 awk命令也可以用来生成伪随机数的
```sh
echo | awk "{ srand(); print rand() }"
```
