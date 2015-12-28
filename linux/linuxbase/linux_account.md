[TOC]
#linux账号管理
##UID、GID
###/etc/passwd
记录用户名、x、UID、GID、用户信息说明，home路径以及sh环境
第二个字段x真的代表没任何意义
###/etc/group
记录用户名、x、GID、此群组支持的账号名称
###/etc/shadow
记录用户名、passwd(加密)、passwd最后一次变动日期、passwd不可变动的天数、passwd需要重新变更的天数、passwd需要变更期限前的警告天数、口令过期后的账号宽限时间(口令失效日)、账号失效日期、保留字段
###Linux对UID的几个限制
| id范围 | 该ID使用者特性 |
| :-------- | :-------- |
| 0(系统管理员) | 代表这个账号是『系统管理员』！ 所以当你要让其他的账号名称也具有 root 的权限时，将该账号的 UID 改为 0 即可。 |
| 1--499(系统账号)|系统保留，其中1--99由distributions字词那个创建，100--499可以由用户使用|
| 500--4294967295(用户账号)| 给一般使用者使用|
##shell登陆流程
1. 先找寻 /etc/passwd 里面是否有你输入的账号？如果没有则跳出，如果有的话则将该账号对应的 UID 与 GID (在 /etc/group 中) 读出来，另外，该账号的家目录与 shell 配置也一并读出；
2. 再来则是核对口令表啦！这时 Linux 会进入 /etc/shadow 里面找出对应的账号与 UID，然后核对一下你刚刚输入的口令与里头的口令是否相符？
3. 如果一切都 OK 的话，就进入 Shell 控管的阶段啰！
##有效群组和初始群组
###初始群组和非初始群组
```sh
# grep dmtsai /etc/passwd /etc/group /etc/gshadow
/etc/passwd:dmtsai:x:503:504::/home/dmtsai:/bin/bash
/etc/group:users:x:100:dmtsai  #<==次要群组的配置
/etc/group:dmtsai:x:504:       #<==因为是初始群组，所以第四字段不需要填入账号
/etc/gshadow:users:::dmtsai    #<==次要群组的配置
/etc/gshadow:dmtsai:!::
```
如上面例子， 账号dmtsai的GID是504，对应的组是dmtsai，这个dmtsai组就是初始群组
在群组users中同样出现了dmtsai账号，这个users群组就是dmtsai的非初始群组。
此时dmtsai具备了dmtsai和users两个群组的权限
###有效群组
```sh
$ groups
dmtsai users
$ touch test
$ ll
-rw-rw-r-- 1 dmtsai dmtsai 0 Feb 24 17:26 test
```
使用groups列出当前登陆用户所在的群组，列出的第一个群组就是有效群组， 此时创建的文件，这个文件的拥有者就是有效群组
###有效群组切换
###newgrp命令
```sh
$ newgrp users
$ groups
users dmtsai
$ touch test2
$ ll
-rw-rw-r-- 1 dmtsai dmtsai 0 Feb 24 17:26 test
-rw-r--r-- 1 dmtsai users  0 Feb 24 17:33 test2
```
####newgrp切换有效群组时需注意的几个问题
1. 该群组已经包含这个账号
2. newgrp只用户临时切换有效群组，输入exit或者关闭shell，变更便会时效
##用户管理
###新建账号useradd
####格式
```sh
# useradd [-u UID] [-g 初始群组] [-G 次要群组] [-mM] [-c 说明栏] [-d home绝对路径] [-s shell] 使用者账号名
```
####常用选项
- -u  ：后面接的是 UID ，是一组数字。直接指定一个特定的 UID 给这个账号；
- -g  ：后面接的那个组名就是我们上面提到的 initial group 啦～该群组的 GID 会被放置到 /etc/passwd 的第四个字段内。
- -G  ：后面接的组名则是这个账号还可以加入的群组。这个选项与参数会修改 /etc/group 内的相关数据喔！
- -M  ：强制！不要创建用户家目录！(系统账号默认值)
- -m  ：强制！要创建用户家目录！(一般账号默认值)
- -c  ：这个就是 /etc/passwd 的第五栏的说明内容啦～可以随便我们配置的啦～
- -d  ：指定某个目录成为家目录，而不要使用默认值。务必使用绝对路径！
- -r  ：创建一个系统的账号，这个账号的 UID 会有限制 (参考 /etc/login.defs)
- -s  ：后面接一个 shell ，若没有指定则默认是 /bin/bash 的啦～
- -e  ：后面接一个日期，格式为『YYYY-MM-DD』此项目可写入 shadow 第八字段，亦即账号失效日的配置项目啰；
- -f  ：后面接 shadow 的第七字段项目，指定口令是否会失效。0为立刻失效，-1 为永远不失效(口令只会过期而强制于登陆时重新配置而已。)
####注意事项
1. linux提供用户模板工useradd使用
2. useradd 创建了账号之后，在默认的情况下，该账号是暂时被封锁的
####查看默认用户模板
```sh
# useradd -D
GROUP=100		#<==默认的群组
HOME=/home		#<==默认的家目录所在目录
INACTIVE=-1		#<==口令失效日，在 shadow 内的第 7 栏
EXPIRE=			#<==账号失效日，在 shadow 内的第 8 栏
SHELL=/bin/bash		#<==默认的 shell
SKEL=/etc/skel		#<==home目录的内容数据参考目录
CREATE_MAIL_SPOOL=no    #<==是否主动帮使用者创建邮件信箱(mailbox)
```
###配置passwd
####passwd
#####格式
```sh
# passwd [-l] [-u] [--stdin] [-S] [-n 日数] [-x 日数] [-w 日数] [-i 日期] 账号
```
#####常用选项
- --stdin ：可以透过来自前一个管线的数据，作为口令输入，对 shell script 有帮助！
- -l  ：是 Lock 的意思，会将 /etc/shadow 第二栏最前面加上 ! 使口令失效；
- -u  ：与 -l 相对，是 Unlock 的意思！
- -S  ：列出口令相关参数，亦即 shadow 文件内的大部分信息。
- -n  ：后面接天数，shadow 的第 4 字段，多久不可修改口令天数
- -x  ：后面接天数，shadow 的第 5 字段，多久内必须要更动口令
- -w  ：后面接天数，shadow 的第 6 字段，口令过期前的警告天数
- -i  ：后面接『日期』，shadow 的第 7 字段，口令失效日期
#####注意事项
1. root可以为所有账号配置passwd
2. 普通账号只能为自己配置passwd
3. 普通账号更改passwd时，需要先输入自己的旧passwd
####chage
chage除了不能修改密码，功能基本上与passwd一致
#####格式
```sh
# chage [-ldEImMW] 账号名
```
#####常用选项
- -l ：列出该账号的详细口令参数；
- -d ：后面接日期，修改 shadow 第三字段(最近一次更改口令的日期)，格式 YYYY-MM-DD
- -E ：后面接日期，修改 shadow 第八字段(账号失效日)，格式 YYYY-MM-DD
- -I ：后面接天数，修改 shadow 第七字段(口令失效日期)
- -m ：后面接天数，修改 shadow 第四字段(口令最短保留天数)
- -M ：后面接天数，修改 shadow 第五字段(口令多久需要进行变更)
- -W ：后面接天数，修改 shadow 第六字段(口令过期前警告日期)
#####注意事项
如果需要用户第一次登陆时，强制更改passwd，可以使用chage命令将最近一次更改passwd的日期修改为0
```sh
# chage -d 0 username
```
###变更账号usermod
####格式
```sh
# usermod [-cdegGlsuLU] username
```
####常用选项
- -c  ：后面接账号的说明，即 /etc/passwd 第五栏的说明栏，可以加入一些账号的说明。
- -d  ：后面接账号的家目录，即修改 /etc/passwd 的第六栏；
- -e  ：后面接日期，格式是 YYYY-MM-DD 也就是在 /etc/shadow 内的第八个字段数据啦！
- -f  ：后面接天数，为 shadow 的第七字段。
- -g  ：后面接初始群组，修改 /etc/passwd 的第四个字段，亦即是 GID 的字段！
- -G  ：后面接次要群组，修改这个使用者能够支持的群组，修改的是 /etc/group 啰～
- -a  ：与 -G 合用，可『添加次要群组的支持』而非『配置』喔！
- -l  ：后面接账号名称。亦即是修改账号名称， /etc/passwd 的第一栏！
- -s  ：后面接 Shell 的实际文件，例如 /bin/bash 或 /bin/csh 等等。
- -u  ：后面接 UID 数字啦！即 /etc/passwd 第三栏的数据；
- -L  ：暂时将用户的口令冻结，让他无法登陆。其实仅改 /etc/shadow 的口令栏。
- -U  ：将 /etc/shadow 口令栏的 ! 拿掉，解冻啦！
###删除账号userdel
####格式
```sh
# userdel [-r] username
```
####常用选项
- -r  ：连同用户的家目录也一起删除

###其他用户管理命令
####finger
查看用户的其他信息，这些信息大多记录在/etc/passwd
#####格式
```sh
$ finger [-s] username
```
#####常用选项
- -s  ：仅列出用户的账号、全名、终端机代号与登陆时间等等；
- -m  ：列出与后面接的账号相同者，而不是利用部分比对 (包括全名部分)
#####信息说明

-   Login：为使用者账号，亦即 /etc/passwd 内的第一字段；
-   Name：为全名，亦即 /etc/passwd 内的第五字段(或称为批注)；
-   Directory：就是家目录了；
-   Shell：就是使用的 Shell 文件所在；
-   Never logged in.：figner 还会调查用户登陆主机的情况喔！
-   No mail.：调查 /var/spool/mail 当中的信箱数据；
-   No Plan.：调查 ~vbird1/.plan 文件，并将该文件取出来说明！
####chfn
changer finger
#####格式
```sh
$ chfn [-foph] [账号名]
```
#####常用选项
- -f  ：后面接完整的大名；
- -o  ：您办公室的房间号码；
- -p  ：办公室的电话号码；
- -h  ：家里的电话号码！
####chsh
变更shell环境
#####格式
```sh
$ chsh [-ls]
```
#####常用选项
- -l  ：列出目前系统上面可用的 shell ，其实就是 /etc/shells 的内容！
- -s  ：配置修改自己的 Shell 
####id
查看默认或自己相关的UID/GID等信息
#####格式
```sh
$ id [username]
```
##组管理
###新建组groupadd
####格式
```sh
# groupadd [-g gid] [-r] 组名
```
####常用选项
- -g  ：后面接某个特定的 GID ，用来直接给予某个 GID
- -r  ：创建系统群组啦！与 /etc/login.defs 内的 GID_MIN 有关
###变更组groupmod
####格式
```sh
# groupmod [-g gid] [-n group_name] 群组名
```
####常用选项
- -g  ：修改既有的 GID 数字
- -n  ：修改既有的组名
###删除组
####格式
```sh
# groupdel [groupname]
```
####注意事项
删除组的时候，需要确认/etc/passwd内没有账号使用该群组作为初始群组，如果，则需要修改这个群组的GID，或者删除这个账号
###组管理员命令gpasswd
通常root负责创建组并指定组管理员，组管理员负责管理组成员
####root
#####格式
```sh
# gpasswd [-A user1,...] [-M user3,...] groupname
```
#####常用选项
-     ：若没有任何参数时，表示给予 groupname 一个口令(/etc/gshadow)
- -A  ：将 groupname 的主控权交由后面的使用者管理(该群组的管理员)
- -M  ：将某些账号加入这个群组当中！
- -r  ：将 groupname 的口令移除
- -R  ：让 groupname 的口令栏失效
####组管理员
#####格式
```sh
$ gpasswd [-ad] user groupname
```
#####常用选项
- -a  ：将某位使用者加入到 groupname 这个群组当中！
- -d  ：将某位使用者移除出 groupname 这个群组当中。
####/etc/gshadow
记录组名、口令栏、群组管理员账号、群组账号(与/etc/group相同)

