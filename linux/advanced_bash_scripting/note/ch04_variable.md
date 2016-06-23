[TOC]
# 变量和参数
## 4.1 变量替换
变量名是其所指向值的一个占位符(placeholder)。而引用这个值的过程我们称之为变量替换(variable substitution)。
### 变量替换符号 $
```sh
bash$ variable1=23
bash$ echo variable1
variable1
bash$ echo $variable1
23
```
> 1. 变量仅仅在声明时、赋值时、被删除时( unset )、被导出时( export ),算术运算中使用双括号结构时或在代表信号时(signal,查看样例 32-5)，在read语句中，在循环头部时才不需要有$ 前缀
> 2. 赋值时用 =
> 3. 实际上 $variable 是 ${variable}的简化形式。在一些使用 $variable 有
语法错误的情况下,使用完整的形式可能会有效

### 部分引用(弱引用)
变量即使在双引号 "" 中被引用也不会影响变量替换，这个被称为部分引用，有时也称为弱引用
## 4.2 变量赋值
### 赋值操作符
=
####一些比较另类的赋值
- a='echo Hello' 以及 a='ls -l'等命令替换结构中，变量中和命令相同的文本将被当做命令执行，但是在命令行中不会有这种情况
- 使用 $(...) 形式进行赋值(与反引号不同的新形式),与命令替换形式相似。
```sh
# 摘自 /etc/rc.d/rc.local
R=$(cat /etc/redhat-release)
arch=$(uname -m)
```
## 4.3 Bash变量是弱类型
- 与许多其他的编程语言相比,Bash并不会区分变量的类型。本质上说,Bash 变量是字符串。
- 但是在一些情况下,Bash允许对变量进行算术运算和比较。而决定因素则是变量值是否只含有数字。
- 如果变量内容为非数字，则对其进行加减乘除没什么卵用
- 如果变量内容为空串或者变量未声明，Bash通常将其值设为整数值0
## 4.4特殊的变量类型
### 局部变量
仅在特定代码块或函数中才可以被访问的变量(参考函数章节的局部变量部分)
### 环境变量
会影响用户及shell行为的变量，临时更新环境变量使用export命令
### 位置参数
使用命令行调用脚本时传递的参数在脚本中使用位置参数调用，即$0, $1, $2, $3 ...
- $0 代表的是脚本名称
- $1 代表第一个参数, $2 代表第二个, $3 代表第三个,以此类推
- 在 $9 之后的参数必须被包含在大括号中,比如${10},${11}, ${12}
- $# 代表传入参数的个数
- 特殊变量 $* 与 $@ 代表所有的位置参数
- 访问最后一个参数可以使用大括号助记符
```sh
args=$#   # 传入参数的个数
lastarg=${!args}
# 这是 $args 的一种间接引用方式
# 也可以使用:  lastarg=${!#}   (感谢 Chris Monson.)
# 这是 $# 的一种间接引用方式。
# 注意 lastarg=${!$#} 是无效的。
```
#### 位置参数移位 shift
- shift命令可以将除$0以外的全体位置参数向左移动一位并重新赋值，如$1 <--- $2 , $2 <--- $3 , $3 <--- $4 ,以此类推
```sh
until	[	-z	"$1"	]		#	直到访问完所有的参数
do
		echo	-n	"$1	"
		shift
done
#	那些被访问完的参数又会怎样呢?
echo	"$2"
#	什么都不会被打印出来。
#	当	$2	被移动到	$1	且没有	$3	时,$2	将会保持空。
#	因此	shift	是移动参数而非复制参数。
```
- shift命令可以指定移动多少位
```sh
shift	3				#	一次移动3位。
#		n=3;	shift	$n
#		的效果相同。
echo	"$1"
exit	0
#	========================	#
$	sh	shift-past.sh	1	2	3	4	5
4
```