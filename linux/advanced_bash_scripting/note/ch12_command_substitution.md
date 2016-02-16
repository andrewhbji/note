[TOC]
# 命令替换
命令替换实际上是将命令的输出导入到另一个地方命令替换通常形式是 `...`，即反引号引用命令，也可以使用$(...)代替反引号s
```sh
script_name=`basename $0`
echo "The name of this script is $scirpt_name."
```
- 命令的输出可以作为另一个命令的参数，也可以复制给一个变量，或者在 for 循环中作为参数列表
- 命令替换的本质是调用一个子shell执行
- 命令替换会出现字串分割的情况，这时需要用引号引用
```sh
COMMAND `echo a b`     # 2 args: a and b
COMMAND "`echo a b`"   # 1 arg: "a b"
COMMAND `echo`         # no arg
COMMAND "`echo`"       # one empty arg
```
- 可以使用 重定向 或者 cat 命令把一个文件的内容通过命令替换赋值给一个变量
```sh
variable1=`<file1`      #  Set "variable1" to contents of "file1".
variable2=`cat file2`   #  Set "variable2" to contents of "file2".
```
>备注：尽量不要讲一大段文字或一个二进制文件的内容赋值给一个变量

- 命令替换允许将循环内的输出结果赋值给一个变量
```sh
variable1=`for i in 1 2 3 4 5
do
  echo -n "$i"                 #  The 'echo' command is critical
done`                          #+ to command substitution here.

echo "variable1 = $variable1"  # variable1 = 12345


i=0
variable2=`while [ "$i" -lt 10 ]
do
  echo -n "$i"                 # Again, the necessary 'echo'.
  let "i += 1"                 # Increment.
done`

echo "variable2 = $variable2"  # variable2 = 0123456789
```
- $(...) 和 反引号 在处理双 反斜杠 上有所不同
```sh
bash$ echo `echo \\`
# 输出
bash$ echo $(echo \\)
# 输出 \
```
- $(...) 允许嵌套
```sh
word_count=$( wc -w $(echo * | awk '{print $8}') )
```
- `...` 也可以嵌套，但是需要将内部的反引号转义
```sh
word_count=` wc -w \`echo * | awk '{print $8}'\` `
```
