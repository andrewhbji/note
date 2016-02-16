#!/bin/bash
# weirdvars.sh: 输出一些奇怪的变量
echo
var="'(]\\{}\$\""
echo $var        # '(]\{}$"
echo "$var"      # '(]\{}$"     没有任何区别。
echo
IFS='\'
echo $var        # '(] {}$"     \ 被转换成了空格,为什么?
echo "$var"      # '(]\{}$"
# 上面的例子由 Stephane Chazelas 提供。
echo
var2="\\\\\""
echo $var2       #   "
echo "$var2"     # \\"
echo
# 而 var2="\\\\"" 不是合法的语法,为什么?
var3='\\\\'
echo "$var3"     # \\\\
# 强引用是可以的。
# ************************************************************ #
# 就像第一个例子展示的那样,嵌套的引用是允许的。
echo "$(echo '"')"           # "
#    ^           ^
# 在有些时候这中方法将会非常有用。
var1="Two bits"
echo "\$var1 = "$var1""      # $var1 = Two bits
#    ^                ^
# 或者,可以像 Chris Hiestand 指出的那样:
if [[ "$(du "$My_File1")" -gt "$(du "$My_File2")" ]]
#     ^     ^         ^ ^     ^     ^         ^ ^
then
  ...
fi
# ************************************************************ #
