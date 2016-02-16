[TOC]
# 11.1 循环
## for 循环
```sh
for arg in [list]
do
  command(s)...
done
```
- list中允许通配符
- 如果 do 和 for 写在同一行时,需要在 list 之后加上一个分号
- 一个单一变量也可以成为 for 循环的list
```sh
FILES="/usr/sbin/accept
/usr/sbin/pwck
/usr/sbin/chroot
/usr/bin/fakefile
/sbin/badblocks
/sbin/ypbind"

for file in $FILES
do
  echo $file
done
```
- list 可以是通配符
```sh
filename="*txt"
for file in $filename
do
 echo "Contents of $file"
 echo "---"
 cat "$file"
 echo
done
```
- in list 缺省时，遍历$@ 位置参数
- 可以使用命令替换生成list
```sh
NUMBERS="9 7 3 8 37.53"
for number in `echo $NUMBERS`  # for number in 9 7 3 8 37.53
do
  echo -n "$number "
done
```
- for 循环的结果可以通过管道导向至一个或多个命令中
```sh
for file in "$( find $directory -type l )"   # -type l = symbolic links
do
  echo "$file"
done | sort    
```
- 类似 C 语言的写法
```sh
for ((a=1; a <= LIMIT ; a++))  # Double parentheses, and naked "LIMIT"
do
  echo -n "$a "
done                           # A construct borrowed from ksh93.
```
- 关键字 do 和 done 圈定了 for 循环代码的范围，但是在一些特殊情况下，也可以被大括号取代
```sh
for((n=1; n<=10; n++))
{
  echo -n "* $n *"
}
```
## while 循环
循环条件为真时，执行循环
```sh
while [ condition ]
do
 command(s)...
done
```
- 将 do 和循环条件放在同一行时需要加一个分号
- while 循环可以有多个测试条件，但只有最后一个条件决定循环是否终止
```sh
#!/bin/bash

var1=unset
previous=$var1

while echo "previous-variable = $previous"
      echo
      previous=$var1
      [ "$var1" != end ] # 当 read end 时，跳出循环
do
echo "Input variable #1 (end to exit) "
  read var1
  echo "variable #1 = $var1"
done

exit 0
```
- while 循环使用双括号结构，可以写的和 c 语言那样
```sh
while (( a <= LIMIT ))
do
  echo -n "$a "
  ((a += 1))
done
```
- 测试条件可以调用函数
- while read 结构，可以用来解析文件
## until 循环
循环条件为真时，退出循环
```sh
until [ condition-is-true ]
do
 command(s)...
done
```
# 11.2 嵌套循环
外层循环会不断的触发内层循环直到外层循环结束。当然,你仍然可以使用 break 可以终止外层或内层的循环
# 11.3 循环控制
## break，continue
- break 跳出循环
- break 可以接受一个参数，指定跳出其上 N 层的循环
```sh
for outerloop in 1 2 3 4 5
do
  echo -n "Group $outerloop:   "

  # --------------------------------------------------------
  for innerloop in 1 2 3 4 5
  do
    echo -n "$innerloop "

    if [ "$innerloop" -eq 3 ]
    then
      break 2 # 直接退出outerloop那一层循环
    fi
  done
  echo
done
# 输出 Group 1:   1 2 3
```
- continue 略过未执行的循环部分
- continue 接受一个参数，指定略过其上 N 层的循环
```sh
for outer in I II III IV V           # outer loop
do
  echo; echo -n "Group $outer: "

  # --------------------------------------------------------------------
  for inner in 1 2 3 4 5 6 7 8 9 10  # inner loop
  do

    if [[ "$inner" -eq 7 && "$outer" = "III" ]]
    then
      continue 2  # Continue at loop on 2nd level, that is "outer loop".
                  # Replace above line with a simple "continue"
                  # to see normal loop behavior.
    fi

    echo -n "$inner "  # 7 8 9 10 will not echo on "Group III."
  done

done
```
# 11.4 测试与分支
## case (in) / esac
```sh
case "$variable" in

 "$condition1" )
 command...
 ;;

 "$condition2" )
 command...
 ;;
esac
```
- 对变量进行引用不是必须的,因为在这里不会进行字符分割。
- 条件测试语句必须以右括号	)	结束。
- 每一段代码块都必须以双分号	;;	结束。
- 如果测试条件为真,其对应的代码块将被执行,而后整个 case 代码段结束执行。
- case 代码段必须以 esac 结束(倒着拼写case)。

## select 结构
```sh
select variable [in list]
do
 command...
 break
done
```
- select 结构可以更加方便的构建菜单，效果是终端会提示用户输入list中的选项，默认是哟欧诺个 $PS3 作为菜单的提示字串
```sh
#!/bin/bash

PS3='Choose your favorite vegetable: ' # Sets the prompt string.
                                       # Otherwise it defaults to #? .

echo

select vegetable in "beans" "carrots" "potatoes" "onions" "rutabagas"
do
  echo
  echo "Your favorite veggie is $vegetable."
  echo "Yuck!"
  echo
  break  # What happens if there is no 'break' here?
done

exit
```
- 如果 in list 被省略，那么 select 将会使用传入脚本的命令行参数 $@ 或者传入函数的参数作为 list
```sh
#!/bin/bash

PS3='Choose your favorite vegetable: '

echo

choice_of()
{
select vegetable
# [in list] omitted, so 'select' uses arguments passed to function.
do
  echo
  echo "Your favorite veggie is $vegetable."
  echo "Yuck!"
  echo
  break
done
}

choice_of beans rice carrots radishes rutabaga spinach
#         $1    $2   $3      $4       $5       $6
#         passed to choice_of() function

exit 0
```
