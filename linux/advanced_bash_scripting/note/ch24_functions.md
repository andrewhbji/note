[TOC]
# 函数
## 函数定义
```sh
function function_name {
command...
}
```
```sh
function_name () {
command...
}
```
```sh
function_name ()
{
command...
}
```
## 调用函数
函数定义不许在第一次调用函数前完成
```sh
function_name ()
{
command...
}
function_name
```
> 备注：
> 1. 函数内可以嵌套另一个函数，只是嵌套函数不能被直接调用

```sh
f1 ()
{

  f2 () # nested
  {
    echo "Function \"f2\", inside \"f1\"."
  }

}  

f2  # 引起错误.
    # 就是你先"declare -f f2"了也没用.

echo    

f1  # 什么也不做,因为调用"f1"不会自动调用"f2".
f2  # 现在,可以正确的调用"f2"了,
    #+ 因为之前调用"f1"使"f2"在脚本中变得可见了.

    # Thanks, S.C.
```
> 2. 函数定义可以出现在一些不可思议的地方，比如管道，if/then结构等等

```sh
ls -l | foo() { echo "foo"; }  # 允许,但没什么用.

if [ "$USER" = bozo ]
then
  bozo_greet ()   # 在 if/then 结构中定义了函数.
  {
    echo "Hello, Bozo."
  }
fi  

bozo_greet        # 只能由 Bozo 运行, 其他用户会引起错误.

# 在某些上下文,像这样可能会有用.
# NO_EXIT=1将会打开下面的函数定义.

[[ $NO_EXIT -eq 1 ]] && exit() { true; }     # 在"and-list"(and 列表)中定义函数.
# 如果 $NO_EXIT 是 1,声明函数"exit ()".
# 把"exit"取别名为"true"将会禁用内建的"exit".

exit  # 调用"exit ()"函数, 而不是内建的"exit".

# 或者, 相似地:
filename=file1

[ -f "$filename" ] &&
foo () { rm -f "$filename"; echo "File "$filename" deleted."; } ||
foo () { echo "File "$filename" not found."; touch bar; }

foo
```
> 3. 函数可以使用下划线，冒号等符号命名
> 4. 可以重复定义函数，但是只会调用最后一次定义的函数

## 24.1 复杂函数和函数的复杂性
### 函数传参
函数通过位置参数来接收参数，调用函数时直接将参数列表附在函数名后面
```sh
function_name(){
  $1... # 处理第一个参数
  $2... # 处理第二个参数
}
function_name $arg1 $arg2
```
> 备注：shell 脚本一般只传递值给函数,变量名(实现上是指针)如果作为参数传递给
函数会被看成是字面上字符串的意思.函数解释参数是以字面上的意思来解释的.

### 传递变量的间接引用
```sh
echo_var ()
{
echo "$1"
}

message=Hello
Hello=Goodbye

echo_var "$message"        # Hello
# Now, let's pass an indirect reference to the function.
echo_var "${!message}"     # Goodbye
```
### 对传递的参数进行接触引用的操作
```sh
my_read () {
  #  用my_read varname这种形式来调用,
  #+ 将之前用括号括起的值作为默认值输出,
  #+ 然后要求输入一个新值.

  local local_var

  echo -n "Enter a value "
# eval 'echo -n "[$'$1'] "'  #  之前的值.
  eval echo -n "[\$$1] "     #  更容易理解,
                             #+ 但会丢失用户在尾部输入的空格.
  read local_var
  [ -n "$local_var" ] && eval $1=\$local_var

  # "与列表": 如果"local_var"的测试结果为true, 则把变量"$1"的值赋给它.
}
my_read var
echo $var # 通过my_read函数修改var的值
```
> 也可以使用全局变量返回字符串或者是数组
```sh
count_lines_in_etc_passwd()
{
  [[ -r /etc/passwd ]] && REPLY=$(echo $(wc -l < /etc/passwd))
  #  If /etc/passwd is readable, set REPLY to line count.
  #  Returns both a parameter value and status information.
  #  The 'echo' seems unnecessary, but . . .
  #+ it removes excess whitespace from the output.
}

if count_lines_in_etc_passwd
then
  echo "There are $REPLY lines in /etc/passwd."
else
  echo "Cannot count lines in /etc/passwd."
fi  

# Thanks, S.C.
```

### 退出和返回
#### 退出状态码
- 退出状态码可以由return命令明确指定
- 也可以使用return来指定成功或者失败(如果成功则返回0, 否则返回非0值)
- 在函数外，调用函数后，可以使用$?来获取引用的退出状态码
#### return
return 命令允许带一个整形参数，这个参数作为函数的退出状态码，并且这个整数也被复制给变量$?
> 退出状态码 大于0小/等与255

### 重定向函数的stdin
函数本质上其实就是一个代码块, 这就意味着它的stdin可以被重定向
```sh
file=/etc/passwd
pattern=$1
file_excerpt ()    #  按照要求的模式来扫描文件,
{                  #+ 然后打印文件相关的部.
  while read line  # "while"并不一定非得有"[ condition ]"不可
  do
    echo "$line" | grep $1 | awk -F":" '{ print $5 }'
    # awk用":"作为界定符.
  done
} <$file  重定向到函数的stdin.
file_excerpt $pattern
```
## 24.2 局部变量
如果变量用local来声明，那么它就只能在被声明的代码快中可见
```sh
local var
var=3
echo var # 3
```
>备注：
>1. 在函数调用前，所有声明在函数中的变量在函数外都不可见，包括使用local声明的变量
>2. local声明变量时，这条命令首先设置变量的值，然后设置变量的作用范围，所以这条命令的返回值应该是设置变量作用范围的结果
```sh
t0=$(exit 1)
echo $? #返回值是exit 1的执行结果，即 1
local t1=$(exit 1)
echo $? #返回值是设置变量作用范围的结果，即 0
local t2
echo $? #返回值是设置变量作用范围的结果，即 0
t2=$(exit 1)
echo $? #返回值是exit 1的执行结果，即 1
```

### 24.2.1 局部变量使递归变为可能
局部变量允许递归, 但是这种方法会产生大量的计算, 因此在shell脚本中, 非常明确的不推荐这种做法.
## 24.3 不使用局部变量的递归
即使不使用局部变量, 函数也可以递归的调用自身.
