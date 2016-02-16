[TOC]
# 处理变量
## 10.1 处理字串
### 字串长度
- ${#string}
```sh
stringZ=abcABC123ABCabc
echo ${#stringZ}                 # 15
```
- expr length $string
```sh
echo `expr length $stringZ`      # 15
```
- expr "$string":'.\*'
```sh
echo `expr "$stringZ" : '.*'`    # 15
```
### 匹配字符串开头的子串长度
- expr match "$string" "$substring"
- expr "$string":"$substring"
### 子串出现的位置
expr index $string $substring
### 提取子串
- ${string:position}
- ${string:position:length}
>备注：
> 1. position < 0 默认提取所有子串
> 2. position < 0 时，${string:(position)}和${string: position}等效于将position当做正数处理
> 3. 如果$string参数是"\*"或者"@"，就会从$position位置开始提取位置参数
```sh
echo ${*:2:3} #打印$2,$3,$4位置参数
echo ${*:2} #打印从$2开始的所有位置参数
echo ${@:2} #同上
```

- expr substr $string $position $length
- expr match "$string" '\\($substring\\)'
从$string的开始位置截取$substring
- expr "$string": '\\($substring\\)'
从$string的开始位置截取$substring
- expr match "$sttring" '.\*\\($substring\\)'
从$string的结束位置截取$substring
- expr "$sttring": '.\*\\($substring\\)'
从$string的结束位置截取$substring
### 子串剔除
- ${string#substring}
从 $string 的开头位置截掉最短匹配的 $substring
- ${string##substring}
从 $string 的开头位置截掉最长匹配的 $substring
- ${string%substring}
从 $string 的结尾位置截掉最短匹配的 $substring
- ${string%%substring}
从 $string 的结尾位置截掉最长匹配的 $substring
### 子串替换
- ${string/substring/replacement}
使用 $replacement 来替换第一个匹配的 $substring
- ${string//substring/replacement}
使用 $replacement 来替换所有匹配的 $substring
- ${string/#substring/replacement}
如果 $substring 匹配 $string 的开头部分, 那么就用$replacement 来替换 $substring
- ${string/%substring/replacement}
如果 $substring 匹配 $string 的结尾部分, 那么就用$replacement 来替换 $substring
### 10.1.1 使用awk处理字串
- awk '{print substr("'"${String}"'",3,4)}' 等价于 echo ${String:2:4}
- awk '{print index("'"${String}"'",$substring)}' 等价于 expr index $string $substring
>备注：Bash的第一个字符是从0开始记录，awk的第一个字符是从1开始记录

## 10.2 参数替换
### 操作和扩展变量
- ${parameter}
与$paramater相同，就是paramater的值
- ${parameter-default}, ${parameter:-default} 如果parameter没有设置，则parameter的默认值是defalut
>备注：当parameter被设置为null的时候，使用":"可以让default替代null，成为为默认值
```sh
username0=
echo "username0 has been declared, but is set to null."
echo "username0 = ${username0-`whoami`}" # (no output)

echo username1 has not been declared.
echo "username1 = ${username1-`whoami`}" # result of whoami

username2=
echo "username2 has been declared, but is set to null."
echo "username2 = ${username2:-`whoami`}" # result of whoami
```

- ${parameter=default}, ${parameter:=default}
同${parameter-default}, ${parameter:-default}
- ${parameter+alt_value},${parameter:+alt_value}
与${parameter-default},${parameter:-default}相反，当parameter被设置，那么重新设置为alt_value
>备注：当parameter被设置为null时，${parameter:+alt_value}不会将parameter重设置为alt_value
```sh
param5=
a=${param5:+xyz}
echo "a = $a"      # a =
```

- ${parameter?err_msg}, ${parameter:?err_msg}
如果parameter没有被设置，就打印err_msg
>备注：当parameter被设置为null时，${parameter:?err_msg}仍会输出err_msg

### 变量长度/删除字串
#### ${#var}，${#array}
- 对于变量(字串),返回变量的长度；对于数组，返回数组第一个元素的长度
> 备注:
> 1. ${#\*} 和 ${#@} 返回位置参数的个数。
> 2. 任意数组 array,  ${#array[\*]} 和 ${#array[@]} 返回数组中元素的个数。

#### ${var#Pattern},${var##Pattern}
- ${var#Pattern} 删除 $var 开头部分匹配到的最短长度的 $Pattern
- ${var##Pattern} 删除 $var 开头部分匹配到的最长长度的 $Pattern
#### ${var%Pattern},${var%%Pattern}
- ${var%Pattern} 删除 $var 结尾部分匹配到的最短长度的 $Pattern
- ${var%%Pattern} 删除 $var 结尾部分匹配到的最长长度的 $Pattern
### 变量扩展/替换子串
#### 截取字串 ${var:pos}，${var:pos:len}
从var的pos位置除截取字串，len指定截取的长度
#### 替换子串 ${var/Pattern/Replacement}，${var//Pattern/Replacement}
- ${var/Pattern/Replacement} 替换第一个匹配Pattern的字串为Replacement
- ${var//Pattern/Replacement} 替换全部匹配Pattern的字串为Replacement
- ${var/#Pattern/Replacement}
替换var开头部分匹配Pattern的字串为Replacement
- ${var/%Pattern/Replacement}
替换var结尾部分匹配Pattern的字串为Replacement
### 匹配变量
${!varprefix*}, ${!varprefix@}
匹配先前声明过所有以 varprefix 作为变量名前缀的变量
```sh
# 这是带 * 或 @ 的间接引用的一种变换形式。
# 在 Bash 2.04 版本中加入了这个特性。
xyz23=whatever
xyz24=
a=${!xyz*}         #  扩展为声明变量中以 "xyz"
# ^ ^   ^           + 开头变量名。
echo "a = $a"      #  a = xyz23 xyz24
a=${!xyz@}         #  同上。
echo "a = $a"      #  a = xyz23 xyz24
echo "---"
abc23=something_else
b=${!abc*}
echo "b = $b"      #  b = abc23
c=${!b}            #  这是我们熟悉的间接引用的形式。
echo $c            #  something_else
```
