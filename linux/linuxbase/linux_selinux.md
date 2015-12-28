[TOC]
#SElinux
##什么是SELinux
SELinux(Security Enhanced Linux), 由[NSA](http://www.nsa.gov/research/selinux/)开发。
Linux传统读取文件系统的方式被称为DAC(Discretionary Access Control,自主式存取控制),基本上就是一句程序的所有者和文件资源的rwx权限来决定有无存取能力。不过，DAC存取控制一些问题：
- root具有最高权限，如果某程序具有root权限，一旦被取得控制权，那么这程序就可以在系统上操作任意资源
- 使用者可以取得程序来改变文件资源的存取权限，如www的htdoc目录如果被置为777，那么就为攻击http服务敞开了大门
NAS针对DAC的这些问题，导入了MAC(Mandatory Access Control,强制访问控制)，即SELinux，可以针对特定的程序与特定的文件资源进行管控，也就是说，即使获取到root权限，使用特定的程序时，该程序能取得的权限不一定是root，而要看当时该程序的配置而定
##SELinux的启动、关闭与查看
###SELinux的模式
- Enforcing： 强制模式，代表 SELinux 运行中，且已经正确的开始限制 domain/type
- Permissive： 宽容模式，代表 SELinux 运行中，不过仅会有警告信息并不会实际限制 domain/type 的存取。这种模式可以运来作为 SELinux 的 debug 之
- Disable： SELinux关闭
###查看当前SELinux的模式
```sh
# getenforce
Enforcing
```
###查看SELinux的策略
```sh
# sestatus [-vb]
选项与参数：
-v  ：检查列於 /etc/sestatus.conf 内的文件与程序的安全性本文内容；
-b  ：将目前政策的守则布尔值列出，亦即某些守则 (rule) 是否要启动 (0/1) 之意；

# sestatus
SELinux status:                 enabled    <==是否启动 SELinux
SELinuxfs mount:                /selinux   <==SELinux 的相关文件数据挂载点
Current mode:                   enforcing  <==目前的模式
Mode from config file:          enforcing  <==配置档指定的模式
Policy version:                 21
Policy from config file:        targeted   <==目前的政策为何？
```
###查看SELinux的配置档
```sh
# vi /etc/selinux/config
SELINUX=enforcing     <==调整 enforcing|disabled|permissive
SELINUXTYPE=targeted  <==目前仅有 targeted 与 strict
```
###启动SELinux
内核是默认启动SELinux的，如果要关闭SELinux，需要Grub配置的内核启动项后面加上selinux=0
```sh
kernel /vmlinuz-2.6.18-92.el5 ro root=LABEL=/1 rhgb quiet selinux=0
```
###enforcing和permission之间切换
```sh
# setenforce [0|1]
选项与参数：
0 ：转成 permissive 宽容模式；
1 ：转成 Enforcing 强制模式
```
##SELinux的运行模式
###SELinux的组成
- 主体(Subject)：进程
- 目标(Object)：文件系统
- 策略(Policy)
  - targeted：针对网络服务限制较多，针对本机限制较少，是默认的政策
  - strict:完整的 SELinux 限制，限制方面较为严格
- 安全性上下文：主体能不能存取目标除了策略指定之外，主体与目标的安全性本文必须一致才能够顺利存取
###安全性上下文(Security Context)
安全性上下文存在于主题程序和目标文件中，目标文件的安全性上下文存放在文件的inode内，程序在内存中，所以程序的安全性上下文自然在内存中
####文件的安全性上下文
- 查看 ls -Z
```sh
$ ls -Z
system_u:object_r:file_t examples.desktop
```
- 格式
```sh
身份识别:角色:类型:级别
```
#####身份识别类型(Identify)
- root： 表示root账号
- system_u： 表示系统程序
- user_u： 表示一般账号
#####角色类型(Role)
- object_r： 代表文件或目录等文件资源
- system_r： 代表程序
#####类型(Type)
- type：在文件资源 (Object) 上面称为类型 (Type)
- domain：在主体程序 (Subject) 则称为领域 (domain)
>备注：domain 需要与 type 搭配，则该程序才能够顺利的读取文件资源

####程序与文件SELinux type栏位的相关性
例如http服务所用到的httpd文件和/var/www/html目录
```sh
# ll -Zd /usr/sbin/httpd /var/www/html
-rwxr-xr-x  root root system_u:object_r:httpd_exec_t   /usr/sbin/httpd
drwxr-xr-x  root root system_u:object_r:httpd_sys_content_t /var/www/html
# ps aux -Z |grep http
root:system_r:httpd_t root   24089 0.2 1.2 22896 9256 ? Ss 16:06 0:00 /usr/sbin/httpd
root:system_r:httpd_t apache 24092 0.0 0.6 22896 4752 ? S  16:06 0:00 /usr/sbin/httpd
root:system_r:httpd_t apache 24093 0.0 0.6 22896 4752 ? S  16:06 0:00 /usr/sbin/httpd
# 两者的角色栏位都是 object_r ，代表都是文件！而 httpd 属於 httpd_exec_t 类型，
# /var/www/html 则属於 httpd_sys_content_t 这个类型
```
1. /usr/sbin/httpd 触发执行后，httpd进程(主体)就具有了httpd这个domain， 而targeted策略定义了httpd domain的规则
2. 由于targeted策略定义httpd被配置为可读取httpd_sys_content_t这个类型的目标文件，因此所有/var/www/html目录下的文件都可以被httpd进程读取
3. 但是如果最终能不能读取到这个缺的数据，还要看rwx是否符合Linux权限的规范
####重置SELinux上下文
#####修改目标的安全性上下文
```sh
# chcon [-R] [-t type] [-u user] [-r role] 文件
# chcon [-R] --reference=范例档 文件
选项与参数：
-R  ：连同该目录下的次目录也同时修改；
-t  ：后面接安全性本文的类型栏位！例如 httpd_sys_content_t ；
-u  ：后面接身份识别，例如 system_u；
-r  ：后面街角色，例如 system_r；
--reference=范例档：拿某个文件当范例来修改后续接的文件的类型！
```
#####恢复目标文件的安全性上下文
```sh
# restorecon [-Rv] 文件或目录
选项与参数：
-R  ：连同次目录一起修改；
-v  ：将过程显示到萤幕上
```
##查看SELinux错误信息
###SELinux所需的服务
####setroubleshoot
这个服务会将关於 SELinux 的错误信息与克服方法记录到 /var/log/messages
#####启动setroubleshoot
```sh
# chkconfig --list setroubleshoot
setroubleshoot  0:off  1:off  2:off 3:on  4:on  5:on  6:off
# 我们的 Linux 运行模式是在 3 或 5 号，因此这两个要 on 即可。

# chkconfig setroubleshoot on
# 关於 chkconfig 我们会在后面章节介绍， --list 是列出目前的运行等级是否有启动，
# 如果加上 on ，则是在启动时启动，若为 off 则启动时不启动。
```
#####查看setroubleshoot信息
```sh
# cat /var/log/messages | grep setroubleshoot
Mar 23 17:18:44 www setroubleshoot: SELinux is preventing the httpd from using
potentially mislabeled files (/var/www/html/index.html). For complete SELinux
messages. run sealert -l 6c028f77-ddb6-4515-91f4-4e3e719994d4
```
####auditd
audit 是稽核的意思，这个 auditd 会将 SELinux 发生的错误信息写入 /var/log/audit/audit.log 中
#####启动auditd
```sh
# chkconfig --list auditd
auditd      0:off  1:off  2:on   3:on   4:on   5:on   6:off
# chkconfig auditd on
# 若 3:off 及 5:off 时，才需要进行！
```
#####查看auditd信息
```sh
# audit2why < /var/log/audit/audit.log
# 意思是，将登录档的内容读进来分析，并输出分析的结果！结果有点像这样：
type=AVC msg=audit(1237799959.349:355): avc:  denied  { getattr } for  pid=24094
comm="httpd" path="/var/www/html/index.html" dev=hda2 ino=654685 scontext=root:s
ystem_r:httpd_t:s0 tcontext=root:object_r:user_home_t:s0 tclass=file
    Was caused by:
       Missing or disabled TE allow rule.
       Allow rules may exist but be disabled by boolean settings; check boolean
settings.
       You can see the necessary allow rules by running audit2allow with this
audit message as input.
```
##SELinux策略详情查看与管理
###SELinux策略详情查看
####列出SELinux策略提供的规则seinfo
```sh
# seinfo [-Atrub]
选项与参数：
-A  ：列出 SELinux 的状态、守则布尔值、身份识别、角色、类别等所有信息
-t  ：列出 SELinux 的所有类别 (type) 种类
-r  ：列出 SELinux 的所有角色 (role) 种类
-u  ：列出 SELinux 的所有身份识别 (user) 种类
-b  ：列出所有守则的种类 (布尔值)
```
####查看指定的规则sesearch
```sh
# sesearch [-a] [-s 主体类别] [-t 目标类别] [-b 布尔值]
选项与参数：
-a  ：列出该类别或布尔值的所有相关信息
-t  ：后面还要接类别，例如 -t httpd_t
-b  ：后面还要接布尔值的守则，例如 -b httpd_enable_ftp_server
```
>备注： 通过查询布尔值可以查到一组和该布尔值相关的主体程序与目标文件资源之间的规则，可以通过修改布尔值来配置这些规则
####布尔值的查询 getsebool
```sh
# getsebool [-a] [布林值条款]
选项与参数：
-a  ：列出目前系统上面的所有布林值条款配置为开启或关闭值
```
####布尔值的修改 setsebool
```sh
# setsebool [-P] 布林值=[0|1]
选项与参数：
-P  ：直接将配置值写入配置档，该配置数据未来会生效的！
```
###默认安全性上下文的查询与修改 semanage
```sh
# semanage {login|user|port|interface|fcontext|translation} -l
# semanage fcontext -{a|d|m} [-frst] file_spec
选项与参数：
fcontext ：主要用在安全性本文方面的用途， -l 为查询的意思；
-a ：添加的意思，你可以添加一些目录的默认安全性本文类型配置；
-m ：修改的意思；
-d ：删除的意思。
```
