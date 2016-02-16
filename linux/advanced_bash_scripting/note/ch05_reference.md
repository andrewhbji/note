[TOC]
#引用
引用相当于将一个字符串用引号括起来。这样做的效果相当于保护了SHELL或者SHELL脚本中重新解释过或者有扩展功能的特殊字符
## 5.1 引用变量
###双引号引用
通常建议使用双引号""引用变量,使用双引号可以防止字符被分割
```sh
List="one two three"
for a in $List  # 用空格将变量分成几个部分。
do
 echo "$a"
done
# one
# two
# three
echo "---"
for a in "$List" # 在单一变量中保留所有空格。
do #  ^  ^
 echo "$a"
done
# one two three
```
- 双引号""中的变量替换符"$", 反引号"\`"，转义符"\\"不能防止被Bash重新解释
- 当字符分割或者出现空白符等问题时，才需要将命令（比如echo）后面的内容用双引号包括
- echo -e "x\ty"中 \t会被解析成制表符
```sh
variable1="a variable containing five words"
COMMAND This is $variable1 # 带上7个参数执行COMMAND命令:
# "This" "is" "a" "variable" "containing" "five" "words"
COMMAND "This is $variable1" # 带上1个参数执行COMMAND命令:
# "This is a variable containing five words"
variable2="" # 空变量。
COMMAND $variable2 $variable2 $variable2
   # 不带参数执行COMMAND命令。
COMMAND "$variable2" "$variable2" "$variable2"
   # 带上3个参数执行COMMAND命令。
COMMAND "$variable2 $variable2 $variable2"
   # 带上1个参数执行COMMAND命令(两个空格)。
# 感谢 Stéphane Chazelas。
```
###单引号引用
可以认为单引号引用为全引用（相对于双引号部分引用而言）。在单引号中，除了单引号'之外，其他所有特殊字符都将会直接按照其字面意思解析
```sh
echo "Why can't I write 's between single quotes"
echo
# 可以采取迂回的方式。
echo 'Why can'\''t I write '"'"'s between single quotes'
# |-------| |----------| |-----------------------|
# 由三个单引号引用的字符串,再加上转义以及双引号包住的单引号组成。
# 感谢 Stéphane Chazelas 提供的例子。
```
## 5.2 转义
转义是一种引用单字符的方法。特殊字符前加转义符 \ 告诉shell按照字面意思解析这个字符
### \n
换行(line feed)
```sh
echo $'\n'           # 新的一行。
```
### \r
回车(carriage return)
### \t
(水平)制表符
### \v
垂直制表符
### \b
退格
### \a
警报、响铃或闪烁
```sh
echo $'\a'           # 警报(响铃)。
                     # 根据不同的终端版本,也可能是闪屏。
```
### \0xx
ASCII码的八进制形式,等价于 0nn ,其中 nn 是数字
```sh
echo -e "\042"       # 打印 " (引号,八进制ASCII码为42)。
```
### \"
转义引号,指代自身
### \$
转义美元号,指代自身
### \\
转义反斜杠,指代自身
