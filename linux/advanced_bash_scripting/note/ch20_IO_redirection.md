[TOC]
# I/O 重定向
I/O 重定向重定向就是捕捉一个文件，命令，程序，脚本，甚至是脚本中的代码，然后将这些的输出作为输入发送到另一个文件，命令，程序或脚本中。
### 文件描述符
每一个打开的文件都会被分配一个文件描述符， stdin (键盘), stdout (屏幕), 和 stderr(错误消息输出到屏幕上)的文件描述符分别是0,1,2(这三个文件默认都是打开的)，其他打开的文件，保留了描述符3到9
### 重定向操作符
#### COMMAND_OUTPUT >
重定向 stdout 到一个文件,如果没有这个文件就创建, 否则就覆盖
#### : > filename
\> 会把文件"filename"截断为 0 长度,如果文件不存在, 那么就创建一个 0 长度的文件(与'touch'的效果相同);: 是一个占位符, 不产生任何输出
#### > filename
同 : > filename，但是在某些 shell 下可能不能工作
#### COMMAND_OUTPUT >>
重定向 stdout 到一个文件，如果文件不存在, 那么就创建它, 如果存在, 那么就追加到文件后边.
#### 1>filename
重定向 stdout 到文件"filename"
#### 1>>filename
重定向并追加 stdout 到文件"filename"
#### 2>filename
重定向 stderr 到文件"filename"
#### 2>>filename
重定向并追加 stderr 到文件"filename"
#### &>filename
将 stdout 和 stderr 都重定向到文件"filename"
#### 2>&1
重定向 stderr 到 stdout
#### i>&j
重定向文件描述符 i 到 j
#### >&j
默认的, 重定向文件描述符 1(stdout)到 j
#### 0< FILENAME,< FILENAME
从文件中接受输入
#### [j]<>filename
为了读写"filename", 把文件"filename"打开, 并且分配文件描述符"j"给它;如果文件"filename"不存在, 那么就创建它;如果文件描述符"j"没指定, 那默认是 fd 0, stdin
```sh
echo 1234567890 > File # 写字符串到"File".
exec 3<> File # 打开"File"并且给它分配 fd 3.
read -n 4 <&3 # 只读 4 个字符.
echo -n . >&3 # 写一个小数点.
exec 3>&- # 关闭 fd 3.
cat File # ==> 1234.67890
```
#### |
管道
### 复合使用重定向
#### 多个输入输出重定向组合
command < input-file > output-file
#### 将多重输出流重定向到一个文件
```sh
ls -yz >> command.log 2>&1 # 将错误选项"yz"的结果放到文件"command.log"中
ls -yz 2>&1 >> command.log # 输出一个错误消息, 但是并不写到文件中
# 如果将 stdout 和 stderr 都重定向，命令执行的顺序会不同
```
### 关闭文件操作符
#### n<&-
关闭输入文件描述符 n
#### 0<&-, <&-
关闭 stdin
#### n>&-
关闭输出文件描述符 n
#### 1>&-, >&-
关闭 stdout
## 20.1 使用 exec
### exec <filename
将stdin 重定向到 filename 文件，这样，后面的输入就都来自这个文件，也可以使用sed 或 awk 分析文件的每一行
```sh
exec 6<&0 # 将文件描述符#6 与 stdin 链接起来.
exec < data-file   # stdin replaced by file "data-file"

read a1            # Reads first line of file "data-file".
read a2            # Reads second line of file "data-file."
exec 0<&6 6<&-
# 现在将 stdin 从 fd #6 中恢复, 因为刚才我们把 stdin 重定向到#6 了,
#+ 然后关闭 fd #6 ( 6<&- ), 好让这个描述符继续被其他进程所使用.
```
### exec >filename
将stdout重定向到指定的文件
```sh
exec 6>&1 # 将 fd #6 与 stdout 相连接.
exec > $LOGFILE # stdout 就被文件"logfile.txt"所代替，这样后面所有命令的输出都重定向到文件logfile
...
...
exec 1>&6 6>&- # 恢复 stdout, 然后关闭文件描述符#6.
```
## 20.2 重定向代码块
### < 操作符
while, until, for 循环，以及 if/then代码块的尾部可以使用 < 进行 IO重定向
```sh
while [ "$name" != Smith ]  # Why is variable $name in quotes?
do
  read name                 # Reads from $Filename, rather than stdin.
  echo $name
  let "count += 1"
done <"$Filename"           # Redirects stdin to file $Filename.
```
## 20.3 应用
例如将零散的信息重新整合成日志或报告
