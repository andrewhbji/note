[TOC]
# 外部过滤器,程序,命令
## 16.1 基本命令
### ls
- -R 递归选项,列出文件树
- -S 按照文件的大小排序
- -t 按照文件的修改时间排序
- -i 显示文件的inode
- -v 按照文件名排序(大写>小写)
- -b 显示转义字符
- -a 显示全部文件
- -A 显示全部文件，但不包括.以及..目录
- -d 仅列出目录本身，而不是列出目录内的文件数据(即只显示一个'.')
- -f 直接列出结果，不排序
- -F 根据文件,目录等信息，附加数据结构，比如*:代表可运行档； /:代表目录； =:代表 socket 文件； |:代表 FIFO 文件；
- -h 将文件容量以人类较易读的方式(例如 GB, KB 等等)列出来
- -I 长数据串列出，包含文件的属性与权限等等数据
- -n 列出 UID 与 GID 而非使用者与群组的名称
- -r 将排序结果反向输出
- -s 显示文件占据的块大小
### cat,tac
- 输出文件内容到stdout
- 当与重定向操作符(>或>>), 一般都是用来将多个文件连接起来
```sh
cat file.1 file.2 file.3 > file.123 # 把三个文件连接到一个文件中
```
- -n 显示行号
- -b 显示行号(不包括空行)
- -v 使用 ^标记法 标记不可打印字符
- -s 将多个空行压缩成一个空行
- tac 逆置 cat 命令
### rev
把每一行中的内容反转, 并且输出到 stdout 上
### cp
文件拷贝
- -a 相当于-pdr
- -r -R 复制整个文件夹
- -d 如果源文件是链接文件，则复制此链接文件的属性而非文件本身
- -f  为强制赋值，即目标文件已经存在且无法开启，则移除后再尝试一次
- -i 如果目标文件已经存在，则覆盖浅先询问是否覆盖
- -l 创建hard link文件
- -p 连同文件的属性一起复制过去
- -s 创建符号连接文件symbolic link
- -u 如果目标文件比源文件旧，升级目标文件
### mv
移动文件
- -f 强制移动
- -i 如果目标文件已经存在，询问是否覆盖
- -u 如果目标文件已经存在，且比源文件新，才会覆盖
### rm
删除文件
- -f 强制删除，不会出现警告信息
- -i 删除前询问是否删除
- -r 递回删除,用于删除目录
>备注：rm将无法删除以破折号开头的文件，解决办法
> 1. 在要删除的文件的前边加上 ./
> 2. 在文件名前边加上"--"
```sh
bash$ rm ./-badname
bash$ rm -- -badname
```

### rmdir
删除空目录
- -p 联通上一级空的目录也一起删除
### mkdir
创建目录
### chmod
更改现存文件的属性
### chattr
修改文件属性(ext2文件系统)
### ln
创建文件链接
- -s 符号链接(软链接)
> 备注：
硬链接的优点是, 原始文件与链接文件之间是相互独立的--如果你删除或者重命名旧文件,那么这种操作将不会影响硬链接的文件,硬链接的文件讲还是原来文件的内容；然而如果你使用软链接的话，当你把旧文件删除或重命名后, 软链接将再也找不到原来文件的内容了. 而软链接的优点是它可以跨越文件系统(因为它只不过是文件名的一个引用, 而并不是真正的数据). 与硬链接的另一个不同是, 一个符号链接可以指向一个目录.

### man,info
查看命令手册
## 16.2 复杂命令
### find
查找文件
- exec COMMAND [{}] \\;
根据find到的文件执行 COMMAND 命令. 如果包含 {}，那么 find 命令的结果将对 {} 进行参数替换
```sh
find -name '*.md' -exec rm {} \; # 删除当前目录下所有.md文件
```
### xargs
给命令传递参数的过滤器，也可以组个多个命令
- xargs 本质是参数替换，可以克服一般情况下使用过多参数导致命令执行失败的情况
- xargs 从stdin或者管道读取数据作为参数，默认 echo 将参数输出出来，并将换行等空白符由空格取代
- -n 选项用来限制传递进 xargs 的参数的个数
- -0 选项结合 find -print0 或 grep -lZ 使用，允许处理包含空白或引号的参数
```sh
find / -type f -print0 | xargs -0 grep -liwZ GUI | xargs -0 rm -f
grep -rliwZ GUI / | xargs -0 rm -f
#上边两行都可用来删除任何包含"GUI"的文件
```
### expr
通用求值表达式 (算数运算,比较操作，字符串操作，逻辑运算)
## 16.3 时间/日期 命令
### date
输出时间,日期
- -u 选项输出UTC时间
- +%j 参数输出今天是本年度的第几天
- +%s 参数输出从1970/01/01至今的秒数
- +%k%M 使用24小时格式来现实当前小时数和分钟数
### zdump
时区dump: 查看特定时区的当前时间
```sh
zdump EST
EST Tue Sep 18 22:09:22 2001 EST
```
### time
输出统计出来的命令执行时间
### touch
- -a 仅修改访问时间(atime)
- -c 修改文件的时间为当前时间，若该文件不存在则不创建新文件(atime,mtime,ctime)
- -d 后面可以接欲修订的日期而不用目前的日期(mtime,ctime)
```sh
touch -d "2 days ago" bashrc # 将日期调整为两天前
```
- -m 仅修改 mtime
- -t 后面可以接欲修订的时间而不用目前的时间，格式为 YYMMDDhhmm
```sh
touch -t 1601010101 bashrc # 日期改为 2016/01/01 1:01
```
### at
一次性执行的任务
### batch
在系统平均负载下降到.8以下的时候执行一次性的任务
### cal
在stdout中输出一个格式比较整齐的日历
### sleep
脚本暂停，参数为秒
### usleep
脚本暂停，参数为微妙
### hwclock,clock
访问或调整硬件时钟
## 16.4 文本处理命令
### sort
排序
- -f 忽略大小写的差异，例如 A 与 a 视为编码相同
- -b 忽略最前面的空格符部分
- -m 将输入内容合并
- -M 以月份的名字来排序
- -n 使用 纯数字 进行排序
- -r 反向排序
- -u 就是 uniq ，相同的数据中，仅出现一行代表
- -t 分隔符，默认是用 [tab] 键来分隔
- -k 以那个区间 (field) 来进行排序的意思
### tsort
拓扑排序, 读取以空格分隔的有序对, 并且依靠输入模式进行排序.
### uniq
删除一个已排序文件中的重复行，这个命令经常出现在sort命令的管道后边
- -c 用来统计每行出现的次数,并把次数作为前缀放到输出行的前面
```sh
$ uniq -c testfile
1 This line occurs only once.
2 This line occurs twice.
3 This line occurs three times.
```
### expand
expand命令将会把每个tab转化为一个空格. 这个命令经常用在管道中
### unexpand
unexpand命令将会把每个空格转化为一个tab. 效果与expand命令相反
### cut
提取文本
- -d 指定字段定位符
- -f 指定域分隔符
```sh
cut -d ' ' -f1,2 /etc/mtab # 使用cut来获得所有mount上的文件系统的列表
uname -a | cut -d" " -f1,3,11,12 # 使用cut命令列出OS和内核版本
grep '^Subject:' read-messages | cut -c10-80 # 从e-mail中提取消息头
```
```sh
# 使用cut命令来分析一个文件
# List all the users in /etc/passwd.

FILENAME=/etc/passwd

for user in $(cut -d: -f1 $FILENAME)
do
  echo $user
done
```
### paste
合并多个文件(以每个文件一列的形式合并到一个文件中)
```sh
$ cat p1.txt
1
2
3
$ cat p2.txt
a
b
c
$ paste p1.txt p2.txt
1    a
2    b
3    c
```
### join
合并两个文件，具有关联数据库的某些特性
```sh
$ cat 1.data
100 Shoes
200 Laces
300 Socks
$ cat 2.data
100 $40.00
200 $1.00
300 $2.00
$ join 1.data 2.data
100 Shoes $40.00
200 Laces $1.00
300 Socks $2.00
```
### head
打印文件头部的内容，默认10行
- -c 输出头n个字符
### tail
输出文件尾部的内容，默认10行
### grep
正则搜索工具
- -A? -B? 关键字所在行的前?行与后?行
- --color=auto 关键字用颜色显示
- -i 忽略大小写
- -w 匹配整个单词
- -l 仅列出符合匹配的文件
- -r 递归搜索
- -n 列出所有匹配行，并显示行号
- -v 显示所有不匹配的行
- -c 只显示匹配到的行数的总数
- -E 使用扩展正则表达式，等价于egrep
- -q 禁用输出
- -F 禁用正则表达式，仅仅按照字符串字面意思搜索
### look
它所搜索的文件必须是已经排过序的单词列表，如果没有指定搜索哪个文件, look命令就默认搜索 /usr/share/dict/words
### sed, awk
使用独立的脚本语言分析文件
### wc
统计文件或I/O流中的单词数量
- -w 统计单词数量
- -l 统计行数量
- -c 统计字节数量
- -m 统计字符数量
- -L 给出文件中最长行的长度
### tr
字符替换过滤器
- -d 删除指定字符
- -s 删除指定字符(除第一个字符外所有的字符)
- -c 将不匹配得字符替换成指定字符
### fold
按照指定的宽度换行
### fmt
一个简单的文件格式器, 通常用在管道中, 将一个比较长的文本行输出进行"折行"
### col
从stdio中过滤 反向换行符，或者将 空格 用 等价的tab来替换
### column
列格式化工具，在合适的位置插入tab
### colrm
删除指定的列
### nl
将文件输出到stdout，并在每个非空行的前面加上连续的行号
### pr
格式化打印过滤器
### gettext
GNU gettext 将程序的输出翻译成不同国家语言，支持C语言以及其他语言和脚本
### msgfmt
产生二进制消息目录
### iconv
文件编码转换
### recode
同iconv
### TeX, gs
TeX和Postscript都是文本标记语言，对print或者格式化后的图像显示进行预拷贝.TeX是排版系统；Ghostscript (gs) 是一个 遵循GPL的Postscript解释器.
### texexec
处理TeX和pdf文件
### enscript
将普通文本转换为PostScript
### groff, tbl, eqn
文本标记和显示格式化语言
### lex, yacc
lex是用于模式匹配的词汇分析产生程序. 在Linux系统上这个命令已经被flex取代了.
yacc工具基于一系列的语法规范, 产生一个语法分析器. 在Linux系统上这个命令已经被bison取代了.
## 16.5 文件与归档命令
### 归档命令
#### tar
归档
- -c 创建
- -x 解压文件
- --delete 删除归档文件中的文件
- -r 将文件添加到现存的归档文件的内部
- -A 将tar文件添加到现存的归档文件的尾部
- -t 列出现存的归档文件中包含的内容
- -u 更新归档文件
- -d 使用指定的文件系统
- -z 用gizp压缩/解压归档文件
- -j 用bzip2压缩归档文件
#### shar
shell归档工具
#### ar
创建和操作归档文件的工具，用于对二进制对象文件库
#### rpm
Red Hat包管理器
- -i 安装 rpm
- -qf 列出文件属于哪个包
- -qa 列出系统上所有安装的rpm 包
#### cpio
拷贝输入输出，可用于拷贝目录树
#### rpm2cpio
解包 rpm 文件 为 cpio 文件
#### pax
pax( portable archive exchange 可移植存档交互工具)，可以替换 tar 和 cpio
```sh
pax -wf daily_backup.pax ~/linux-server/files
#  Creates a tar archive of all files in the target directory.
#  Note that the options to pax must be in the correct order --
#+ pax -fw     has an entirely different effect.

pax -f daily_backup.pax
#  Lists the files in the archive.

pax -rf daily_backup.pax ~/bsd-server/files
#  Restores the backed-up files from the Linux machine
#+ onto a BSD one.
```
### 压缩命令
#### gzip
压缩工具
- -c 将gzip的输出打印到stdout
- -d 解压缩
#### bzip2
压缩工具
#### compress, uncompress
旧的压缩/解压缩工具
#### sq, unsq
另一种压缩解压缩/解压缩工具，效率不及gzip
#### zip, unzip
跨平台的文件归档和压缩工具, 兼容dos下的pkzip
#### unarc, unarj, unrar
兼容doc下的arc，arj，rar
#### lzma, unlzma, lzcat
支持LZMA（Lempel-Ziv-Markov chain-Algorithm的缩写）高效压缩，[7-zip](http://www.7-zip.org/sdk.html)支持该压缩方式
#### xz, unxz, xzcat
一种新的高效压缩工具，向后兼容lzma，调用语法和gzip相似
### 文件信息
#### file
查看文件类型
- -f 分析该选项指定的文件，从中读取需要处理的文件列表
- -z 分析压缩文件的类型
#### which
列出 command 的完整路径
#### whereis
列出 command 的完整路径和 man 页的完整路径
#### whatis
在whatis数据库中查询命令或者文件
#### vdir
等价于 ls -l
#### locate, slocate
在预先简历好的数据库中查询文件，slocate是locate的安全版本
#### getfacl, setfacl
获取/设置facl(file access control list 文件访问控制列表--owner，group以及权限)
```sh
$ getfacl *
# file: test1.txt
# owner: bozo
# group: bozgrp
user::rw-
group::rw-
other::r--

# file: test2.txt
# owner: bozo
# group: bozgrp
user::rw-
group::rw-
other::r--

$ setfacl -m u:bozo:rw yearly_budget.csv
$ getfacl yearly_budget.csv
# file: yearly_budget.csv
# owner: accountant
# group: budgetgrp
user::rw-
user:bozo:rw-
user:accountant:rw-
group::rw-
mask::rw-
other::r--
```
#### readlink
显示符号链接指向的文件
#### strings
在二进制或者数据文件中查找ASCII编码
### 文件对比
#### diff
对比文件/产生patch文件
- --side-by-side 按照左右分隔的形式，将比较中的文件全部输出，并标记不同的行
- -b 忽略一行当中有多个空格的差异(例如 "about me" 与 "about     me" 视为相同
- -B 忽略空白行的差异
- -e 输出ed脚本
- -i 忽略大小写的不同
- -r 递归比较目录下的所有文件
- -c 和 -u 选项也会使得diff命令的输出变得容易解释一些
#### patch
通过升级/还原文件
- -e 接受ed或ex脚本
- -p 后面可以接 取消几层目录 的意思
- -R 代表还原，将新的文件还原成原来旧的版本
#### diff3
可以对比三个文件
#### merge
精简版本的patch命令
```sh
$ merge Mergefile file1 file2 #The result is to output to Mergefile the changes that lead from file1 to file2.
```
#### sdiff
比较和(或)编辑两个文件，由于这个命令的交互特性，所以脚本中很少使用这个命令
#### cmp
diff　的简化版本，diff仅会指出不同的位置，不会展示细节
-s 将所有的不同点的位组处都列出来,因为 cmp 默认仅会输出第一个发现的不同点。
> 如果两个文件相同，然会0作为退出状态码，否则就返回1

#### comm
多功能的文件比较工具，使用这个命令之前必须先排序
```sh
comm -options first-file second-file
```
- comm file-1 file-2 将会输出3列
  - 第1列 = 只在 file-1 中存在的行
  - 第2列 = 只在 file-2 中存在的行
  - 第3列 = 两边相同的行
- -1 禁止显示第 1 列
- -2 禁止显示第 2 列
- -3 禁止显示第 3 列
- -12 禁止第 1 列和第 2 列
### 工具命令
#### basename
- 从文件名中去掉路径名
- basename $0结构可以让脚本获得它自己的名字
#### dirname
从带路径的文件名字串中去掉文件名，只打印路径信息
#### split, csplit
分割文件，csplit 会根据上下文来分割文件，分割的位置将会发生在模式匹配的地方
- -b 后面可接欲分割成的文件大小，可加单位，例如 b, k, m 等
- -l 以行数来进行分割
### 编码和加密
#### sum, cksum, md5sum, sha1sum
用来产生总和校验码(checksum)，checksum用于校验文件的完整性。
- cksum 将会输出目标文件的大小，单位为byte
- 对于由安全需求的应用，应使用shalsum产生总和校验码
#### uuencode
将二进制文件编码成ASCII，适用于编码e-mail消息体
#### uudecode
将ASCII解码为二进制
#### mimencode, mmencode
用来处理多媒体编码的email附件。虽然mail客户端(UA)通常情况下都会自动处理，但是这些特定的工具允许从命令行或shell脚本中来操作这些附件
#### crypt
加密UNIX文件，正式的crypt被美国政府禁止出口，Linux中的一般是替代方案
#### openssl
安全Socket层加密的开源实现
```sh
$ openssl aes-128-ecb -salt -in file.txt -out file.encrypted -pass pass:my_password # 使用 aes-128-ecb加密文件
$ openssl aes-128-ecb -d -salt -in file.encrypted -out file.txt -pass pass:my_password # 使用aes-128-ecb 解密文件
```
- 可以结合tar命令对归档文件进行加/解密
```sh
# To encrypt a directory:
sourcedir="/home/bozo/testfiles"
encrfile="encr-dir.tar.gz"
password=my_secret_password

tar czvf - "$sourcedir" |
openssl des3 -salt -out "$encrfile" -pass pass:"$password"
#       ^^^^   Uses des3 encryption.
# Writes encrypted file "encr-dir.tar.gz" in current working directory.

# To decrypt the resulting tarball:
openssl des3 -d -salt -in "$encrfile" -pass pass:"$password" |
tar -xzv
# Decrypts and unpacks into current working directory.
```
#### shred
安全擦除文件(在删除前，通过多次使用随机位覆盖文件的方式)
>先进的法医技术可以恢复使用shred擦除的文件

### 其他
#### mktemp
使用唯一的文件名那个来创建一个临时文件，不带参数时，将会在/tmp目录下产生一个随机命名的临时文件
#### make
构建和编译二进制包
#### install
特殊目的的文件拷贝命令，具有拷贝文件权限的能力，一般用于安装软件包。这个命令经常出现在Makefile中(make install区域)，安装脚本中也可以蛋刀这个命令
#### dos2unix
将DOS格式的文本(以CR-LF为行结束符)转换为UNIX格式(以LF为行结束符)
#### ptx
ptx [targetfile] 输出一个顺序索引(对照参考清单)
#### more, less
分页显示文件或stdout
## 16.6 通讯命令
### 信息与统计
#### host
通过域名或IP地址来搜索互联网主机
#### ipcalc
显示主机的IP地址
- -h 选项可以通过IP找域名
#### nslookup
查找互联网上的主机
#### dig
查找互联网上的主机域名
- +time=N 选项用来设置查询超时为N 秒
- +nofail 选项用来持续查询服务器直到收到一个响应
- x 做反向地址查询
#### traceroute
追踪包发送到远端主机的路由信息
#### ping
网络诊断
#### whois
DNS查询
- -h 允许指定需要查询的特定whois服务器
#### finger
获取网络上用户的信息
#### chfn
修改finger命令所显示的用户信息
#### vrfy
验证一个互联网的e-mail地址
### 远程主机访问
#### sx, rx
使用xmodem协议，设置服务和远端主机收发文件
#### sz, rz
使用zmodem协议，设置服务和远端主机收发文件
#### ftp
ftp上传或下载的工具
#### uucp, uux, cu
- uucp UNIX到UNIX拷贝
- uux UNIX到UNIX执行
- cu Call UP 一个远端系统，并作为一个简单终端进行连接
#### telnet
连接远端主机的工具和协议
#### wget
从web或ftp站点上下载文件
#### lynx
网页浏览器，也是一个文件浏览器
- -traversal lynx将会从参数中指定的HTTP URL开始,"遍历"指定服务器上的所有连接
- -crawl 选项一起用的话, 将会把每个输出的页面文本都放到一个log文件中
#### rlogin
远端登陆，已被ssh替代
#### rsh
远端登陆，已被ssh替代
#### rcp
远端拷贝
#### rsync
远端同步
#### ssh
安全shell
#### scp
安全拷贝
### 本地网络
#### write
这是一个端到端通讯的工具，可以从一个终端发送正行数据到另一个终端上
#### netconfig
配置网络适配器(centos专用)
### 邮件
#### mail
发送或读取e-mail
#### mailto
mailto可以发送e-mail以及MIME(多媒体)消息
#### mailstats
显示mail统计,仅root可用
```sh
$ mailstats
Statistics from Tue Jan  1 20:32:08 2008
  M   msgsfr  bytes_from   msgsto    bytes_to  msgsrej msgsdis msgsqur  Mailer
  4     1682      24118K        0          0K        0       0       0  esmtp
  9      212        640K     1894      25131K        0       0       0  local
 =====================================================================
  T     1894      24758K     1894      25131K        0       0       0
  C      414                    0
```
#### vacation
自动恢复e-mail，这个工具不支持POPmail账号
## 16.7 终端控制命令
### tput
初始化终端或者从 terminfo 数据中取得终端信息
### infocmp
打印当前终端信息
### reset
复位终端参数并清屏
### clear
清屏
### resize
重置 COLUMNS 和 LINES 这两个变量
### script
这个工具将会记录(保存到一个文件中)所有的用户按键信息(在控制台下的或在xterm window下的按键信息). 这其实就是创建了一个会话记录.
## 16.8 数字计算命令
### 操作数字
#### factor
将一个整数分解为多个素数，素数只能被分解为一个素数
#### bc
处理浮点运算，参考monthlypmt.sh,base.sh,bc.sh,cannon.sh
#### dc
处理浮点运算，参考factr.sh,hexconvert.sh
#### awk
awk也可以处理浮点运算
## 16.9 其他命令
### jot，seq
生成序列
### getopt
分析以 - 为开头的命令选项，与getopts内建命令作用相同
### run-parts
执行目录下的所有脚本
### yes
默认不断向stdout连续不断的输出字符y，每个 y 单独占一行，是expect命令的简化版本，一般用来通过管道传递给一些需要交互输入的命令
```sh
$ yes | rm -r dirname # 与 rm -rf dirname 等价
```
### banner
将会把传递进来的参数字符串用一个ASCII字符(默认是'#')给画出来(就是将多个'#'拼出一副字符的图形), 然后输出到 stdout
### printenv
显示某个特定用户所有的环境变量
```sh
$ printenv | grep HOME
HOME=/home/bozo
```
### lp
lp和lpr命令将会把文件发送到打印队列中, 并且作为硬拷贝来打印.
### tee
将命令或者管道命令的输出抽出到一个文件中，且不影响结果
```sh
$ cat listfile* | sort | tee check.file | uniq > result.file
# 在对排序的结果进行uniq(去掉重复行)之前, 文件 check.file 保存了排过序的"listfiles"
```
### mkfifo
可以创建一个命名管道，在两个进程之间传递数据
### pathchk
检查文件名的有效性，但是这个命令不能返回一个可识别的错误码，需要用文件测试操作代替
### dd
复制数据，在复制过程中可以进行一些转换
```sh
$ dd if=$filename conv=ucase > $filename.uppercase # 转换为大写
```
- if=INFILE，INFILE是源文件
- of=OUTFILE，OUTFILE是dd写入的目标文件
- bs=BLOCKSIZE，是每一个数据块的大小
- skip=BLOCKS，指定开始复制前，在INFILE中跳过多少个数据块
- seek=BLOCKS，指定复制时在OUTFILE跳过多少个数据块
- count=BLOCKS，仅仅复制指定的数据块，而不是整个INFILE
- conv=CONVERSION，对INFILE进行转换操作的类型
- dd 可以对数据流做随机的访问
```sh
echo -n . | dd bs=1 seek=4 of=file conv=notrunc
#  The "conv=notrunc" option means that the output file
#+ will not be truncated.

# Thanks, S.C.
```
- dd 可以赋值raw数据和硬盘镜像到其他设备，比如软盘
```sh
dd if=kernel-image of=/dev/fd0H1440 # 创建启动软盘
```
- dd 可以从软盘复制内容到其他设备
```sh
dd if=/dev/fd0 of=/home/bozo/projects/floppy.img
```
- dd 可以创建启动U盘或者SD卡
```sh
dd if=image.iso of=/dev/sdb
```
- dd 甚至可以对硬盘进行底层操作，如初始化临时交换分区和内存虚拟硬盘(ramddisks)，对整个硬盘分区进行底层拷贝
### od
octal dump过滤器，将会把输入或文件转换为八进制或其他进制
### hexdump
将二进制文件转换成16进制，8进制，10进制
### objdump
显示编译后的二进制文件或二进制科执行文件的信息，以16进制的形式显示，或者显示反汇编列表(-d选项)
### mcookie
产生magic cookie(128-bit(32-字符)的伪随机16进制数字),这个数
字一般都用来作为X server的鉴权"签名",以及生成随机数
### units
转换不同的计量单位
### m4
宏处理过滤器
```sh
#!/bin/bash
# m4.sh: Using the m4 macro processor

# Strings
string=abcdA01
echo "len($string)" | m4                            #   7
echo "substr($string,4)" | m4                       # A01
echo "regexp($string,[0-1][0-1],\&Z)" | m4      # 01Z

# Arithmetic
var=99
echo "incr($var)" | m4                              #  100
echo "eval($var / 3)" | m4                          #   33

exit
```
### xmessage
弹出xwindow窗口
```sh
$ xmessage Left click to continue -button okay # xwindow 消息为“Left click to continue”，button为okay
```
### zenity
显示gtk对话框
### doexec
允许将随便一个参数列表传递到一个二进制科执行文件中
### dialog
调用对话框
### sox
对声音文件转换
```sh
sox soundfile.wav soundfile.au # 将wav文件转换成au文件
```
