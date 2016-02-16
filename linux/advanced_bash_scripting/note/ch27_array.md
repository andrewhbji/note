[TOC]
# 数组
Bash支持一维数组
## 声明，初始化
```sh
declare -a array # 声明数组
array[11]=23 # 初始化数组的第11个元素的值为23，可以跳过声明直接初始化
echo ${array[11]}    #  提取array指定元素

array2=(zero one two three four) # 初始化五个元素
echo ${array2[0]}    #  提取数组第一个元素

array3=([17]=seventeen [24]=twenty-four) # 初始化多个元素
echo ${array3[17]} # 提取array3指定元素
```
>备注:
>1. 数组成员不一定非得是相邻或连续的
>2. 数组的部分成员可以不被初始化
>3. 数组中允许空缺元素
>4. 实际上, 保存着稀疏数据的数组("稀疏数组"),在电子表格处理软件中是非常有用的
>5. Bash允许将变量当做数组来操作

```sh
string=abcABC123ABCabc
echo ${string[@]}               # abcABC123ABCabc
echo ${string[*]}               # abcABC123ABCabc
echo ${string[0]}               # abcABC123ABCabc
echo ${string[1]}               # 没有输出!
                                # 为什么?
echo ${#string[@]}              # 1
                                # 数组中只有一个元素
                                # 就是这个字符串本身
```
## 数组操作
### 提取指定元素
```sh
echo ${array[0]}       # 提取第一个元素
```
### 提取指定元素的某个部分
```sh
echo ${array:0}        # 提取第一个元素，从第0个位置开始提取
echo ${array:1}        # 提取第一个元素，从第1个位置开始提取
echo ${array[1]:0}     # 提取第二个元素，从第0个位置开始提取
```
### 提取数组所有元素
```sh
echo ${array[*]}
echo ${array[@]}
```
### 数组元素个数
```sh
echo ${#array[*]}
echo ${#array[@]}
```
### 数组元素的长度
```sh
echo ${#array[0]} #  第一个数组元素的长度
echo ${#array} #  第一个数组元素的长度
echo ${#array[1]} #  第二个数组元素的长度
```
### 提取多个指定元素
```sh
echo ${array[@]:0} # 提取所有元素
echo ${array[@]:1} # 提取除第一个元素之外的所有元素
echo ${array[@]:1:2} # 只提取第一个元素后边的两个元素
```
### 子串替换
```sh
echo ${array[@]/fiv/XYZ} # 第一个匹配到的子串将会被替换为XYZ
echo ${array[@]//iv/YY}  # 所有匹配到的子串都会被替换为YY
echo ${array[@]/#fi/XY} # 替换字符串前端子串，，即将fi**中的fi替换为XY
echo ${array[@]/%ve/ZZ} # 替换字符串后端子串，即将*ve中的ve替换为ZZ
```
### 子串/元素删除
```sh
echo ${array[@]//fi/}   # 如果没有指定替换字符串的话, 那就意味着'删除'
unset colors[1] # 删除数组的第2个元素
unset colors # 删除整个数组
echo ${array[@]#new} # 删除和new匹配的子串
echo ${array[@]/new/} # 删除和new匹配的子串
```
>备注：如果${array[@]/new/}删除子串时匹配的是整个元素，那么这个元素将被删除

### 数组入栈
```sh
array=( "${array0[@]}" "new1" )
array[${#array0[*]}]="new1" # 将new1添加到数组末尾
```
### 数组出栈
```sh
unset array[${#array[@]}-1] # 将数组最后一个元素删除
```
### 嵌套数组与间接引用
```sh
ARRAY1=(
        VAR1_1=value11
        VAR1_2=value12
        VAR1_3=value13
)

ARRAY2=(
        VARIABLE="test"
        STRING="VAR1=value1 VAR2=value2 VAR3=value3"
        ARRAY21=${ARRAY1[*]}
)       # ARRAY1 嵌入 ARRAY2

function print(){
  OLD_IFS="$IFS"
  IFS=$'\n'
  TEST1="ARRAY2[*]"
  local ${!TEST1} # 间接引用ARRAY2所有元素
  IFS="$OLD_IFS"
  TEST2="STRING[*]"
  local ${!TEST2} # 间接引用STRING所有元素
  echo "var2 : $VAR2" # 引用VAR2
}
print
exit 0
```
