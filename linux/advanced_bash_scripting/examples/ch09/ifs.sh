#!/bin/bash
# ifs.sh


var1="a+b+c"
var2="d-e-f"
var3="g,h,i"

IFS=+
# + 被解释成分隔符
echo $var1     # a b c
echo $var2     # d-e-f
echo $var3     # g,h,i

echo

IFS="-"
# + 恢复默认
# - 被解释成分隔符
echo $var1     # a+b+c
echo $var2     # d e f
echo $var3     # g,h,i

echo

IFS=","
# , 被解释称分隔符
# - 恢复默认
echo $var1     # a+b+c
echo $var2     # d-e-f
echo $var3     # g h i

echo

IFS=" "
# 空格 给解释成分隔符
# , 恢复默认
echo $var1     # a+b+c
echo $var2     # d-e-f
echo $var3     # g,h,i

# ======================================================== #

# 但是 ...
# $IFS 处理空白的方法,与处理其它字符不同.

output_args_one_per_line()
{
  for arg
  do
    echo "[$arg]"
  done #  ^    ^   嵌入在中括号内，使输出更为明显
}

echo; echo "IFS=\" \""
echo "-------"

IFS=" "
var=" a  b c   "
#    ^ ^^   ^^^
output_args_one_per_line $var  # output_args_one_per_line `echo " a  b c   "`
# [a]
# [b]
# [c]


echo; echo "IFS=:"
echo "-----"

IFS=:
var=":a::b:c:::"               # 同上,
#    ^ ^^   ^^^                #+  ":" 替换 " "  ...
output_args_one_per_line $var
# []
# [a]
# []
# [b]
# [c]
# []
# []

# 备注 "empty" 中括号.
# 同样的事情也会发生在 awk 中的"FS"域分隔符.


echo

exit
