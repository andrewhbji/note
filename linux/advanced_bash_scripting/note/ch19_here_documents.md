[TOC]
# Here Documents
here document 使用 I/O重定向的形式来将一个命令序列传递到一个交互程序或者命令中
```sh
interactive-program <<LimitString
command #1
command #2
...
LimitString
```
- LimitString 用来划定命令序列的范围
- << 用来标示LimitString，这个符号具有重定向文件的输出或命令的输入到程序的作用
- vi、ex等文本编辑器 也可以使用here document
```sh
vi $TARGETFILE <<x23LimitStringx23
i
This is line 1 of the example file.
This is line 2 of the example file.
^[
ZZ
x23LimitStringx23
#----------End here document-----------#
#  i 表示进入编辑模式
#  Note that ^[ above is a literal escape
#+ typed by Control-V <Esc>.
#  ZZ 会退出vi

ex $word <<EOF
:%s/$ORIGINAL/$REPLACEMENT/g
:wq
EOF
# :%s is the "ex" substitution command.
# :wq is write-and-quit.
```
- 类似的还有cat、echo命令
```sh
cat <<End-of-message
-------------------------------------
This is line 1 of the message.
This is line 2 of the message.
This is line 3 of the message.
This is line 4 of the message.
This is the last line of the message.
-------------------------------------
End-of-message
# 使用下面这行替代第一行
#+ cat > $Newfile <<End-of-message
#+ 可以将输出写入到文件$Newfile中
echo "-------------------------------------
This is line 1 of the message.
This is line 2 of the message.
This is line 3 of the message.
This is line 4 of the message.
This is the last line of the message.
-------------------------------------"
```
- 使用 - 来标记LimitString可以抑制输出是前面的tab
```sh
cat <<-ENDOFMESSAGE
	This is line 1 of the message.
	This is line 2 of the message.
	This is line 3 of the message.
	This is line 4 of the message.
	This is the last line of the message.
ENDOFMESSAGE
```
- Here Document的内容也可以进行参数替换和命令替换
```sh
cat <<Endofmessage

Hello, there, $NAME.
Greetings to you, $NAME, from $RESPONDENT.

# This comment shows up in the output (why?).

Endofmessage
```
- 引用或转义开头的LimitString会使得禁用here document的参数替换
```sh

cat <<'Endofmessage'

Hello, there, $NAME.
Greetings to you, $NAME, from $RESPONDENT.

Endofmessage
# 下面这几种方式都可以禁用参数替换
# cat <<'Endofmessage'
# cat <<"Endofmessage"
# cat <<\Endofmessage
```
- 可以将here document的输出保存到变量中
```sh
variable=$(cat <<SETVAR
This variable
runs over multiple lines.
SETVAR
)

echo "$variable"
```
- 同一个脚本中可以接受here document的输出作为自身的参数
```sh
GetPersonalData ()
{
  read firstname
  read lastname
  read address
  read city
  read state
  read zipcode
} # This certainly appears to be an interactive function, but . . .


# Supply input to the above function.
GetPersonalData <<RECORD001
Bozo
Bozeman
2726 Nondescript Dr.
Bozeman
MT
21226
RECORD001
```
- 匿名的here document，使用这样的方式可以为代码添加多行注释
```sh
#!/bin/bash

: <<'TESTVARIABLES'
${HOSTNAME?}${USER?}${MAIL?}  # Print error message if one of the variables not set.
TESTVARIABLES

exit $?
```
> 备注：
> 1. 最后一个LimitString的前后不能有空格符，否则会出现异常
> 2. Here document 创建临时文件, 但是这些文件将在打开后被删除, 并且不能够被任何其他进程所存取
```sh
$ bash -c 'lsof -a -p $$ -d0' << EOF
>EOF
lsof
1213 bozo
0r
REG
3,5
0 30386 /tmp/t1213-0-sh (deleted)
```

## 19.1 Here Strings
here string 可以被认为是 here document 的一种定制形式. 除了 COMMAND <<< $WORD 就什么都没有了, $WORD 将被扩展并且被送入 COMMAND 的 stdin 中
```sh

if grep -q "txt" <<< "$VAR" # 与 if echo "$VAR" | grep -q 等价 txt
then   #         ^^^
   echo "$VAR contains the substring sequence \"txt\""
fi
```
- 可以将 here string 送入循环的stdin中
```sh
ArrayVar=( element0 element1 element2 {A..D} )

while read element ; do
  echo "$element" 1>&2
done <<< $(echo ${ArrayVar[*]})
```
