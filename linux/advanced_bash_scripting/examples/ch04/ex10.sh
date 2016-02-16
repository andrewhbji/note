#!/bin/bash
# 非引用形式变量
echo
# 什么时候变量是非引用形式,即变量名前没有 '$' 符号的呢?
# 当变量在被赋值而不是被引用时。
# 赋值
a=879
echo "The value of \"a\" is $a."
# 使用 'let' 进行赋值
let a=16+5
echo "The value of \"a\" is $a."
echo
# 在 'for' 循环中赋值(隐式赋值)
echo -n "Values of \"a\" in the loop are: "
for a in 7 8 9 11
do
  echo -n "$a "
done
echo
echo
# 在 'read' 表达式中(另一种赋值形式)
echo -n "Enter \"a\" "
read a
echo "The value of \"a\" is now $a."
echo
exit 0
