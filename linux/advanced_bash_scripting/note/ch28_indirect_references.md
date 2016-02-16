[TOC]
# 变量间接引用
假设a=letter_of_alphabet并且letter_of_alphabet=z,那么如何通过a获得z？通过间接引用就可以做到，即eval \\$$a 或者 ${!a} (Bash2.0后支持)获得的值就是z
```sh
letter_of_alphabet=z
a=letter_of_alphabet
eval echo \$$a
# 或者 echo ${!a}
```
>备注：间接引用类似C语言的指针，在Bash中间接引用进行多步处理，首先使用变量名varname引用varname指向的变量：$varname，然后引用varname指向的变量$$varname，接着规避一个第一个$:\\$$varname,最后使用eval执行这个表达式：eval \\$$a
