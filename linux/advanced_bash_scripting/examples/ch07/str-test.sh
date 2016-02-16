
#!/bin/bash
#  str-test.sh: 测试 null 字符串和非引用字符串
#+ but not strings and sealing wax, not to mention cabbages and kings . . .
#+ 上边这句没看懂

# Using   if [ ... ]

# 如果一个字符串没被初始化,那么它就没有定义的值(像这种话,总感觉像屁话)
# 这种状态叫做"null"(与 zero 不同)

if [ -n $string1 ]    # $string1 没被声明和初始化
then
  echo "String \"string1\" is not null."
else
  echo "String \"string1\" is null."
fi                    # 错误的结果
# 显示$string1 为非空,虽然他没被初始化

echo

# 让我们再试一下

if [ -n "$string1" ]  # 这次$string1 被引用了
then
  echo "String \"string1\" is not null."
else
  echo "String \"string1\" is null."
fi                    # ""的字符串在[]结构中

echo

if [ $string1 ]       # 这次$string1 变成"裸体"的了
then
  echo "String \"string1\" is not null."
else
  echo "String \"string1\" is null."
fi                    # 这没什么问题.
# 这个[]test 操作检测 string 是否为 null
# 然而,使用("if [ $string1 ]")是一种很好的习惯
#
# As Stephane Chazelas points out,
#    if [ $string1 ] 有 1 个参数 "]"
#    if [ "$string1" ] 有 2 个参数,空的"$string1"和"]"


echo


string1=initialized

if [ $string1 ]       # 再来,$string1"裸体了"
then
  echo "String \"string1\" is not null."
else
  echo "String \"string1\" is null."
fi                    # 再来,给出了正确的结果.
# 不过怎么说("$string1")还是好很多,因为. . .


string1="a = b"

if [ $string1 ]       # 再来,$string1 再次裸体了.
then
  echo "String \"string1\" is not null."
else
  echo "String \"string1\" is null."
fi                    # 非引用的"$string1"现在给出了一个错误的结果

exit 0   # 也感谢Florian Wisser, 给出了上面这个"足智多谋"的例子.
