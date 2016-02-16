[TOC]
# 系统和管理命令
## 用户和用户组
### users
显示所有登陆用户
### groups
列出当前用户和他所属的组
### chown,chgrp
修改一个过多个文件的所有权(用户和组)
- --recursive 递归修改所有子目录
### useradd,userdel
添加/删除用户账号
### usermod
修改用户账号(密码，组身份，截止日期)
### groupmod
修改用户组
### id
列出当前进程的真是和有效用户ID、用户组ID
### lid
### who
显示系统上所有已经登陆的用户
- -m 列出详细信息
### w
显示所有的登陆的用户和属于他们的进程
### logname
显示当前用户的登录名
### su
切换到root
### sudo
以root身份运行一个命令
### passwd
设置、修改、或管理用户的密码
### ac
显示用户从登陆到现在的时间，和/var/log/wtmp中读取的一样
### last
用户最后登陆的信息
### newgrp
修改用户的组ID(直接生效，不需要登出)
## 终端命令
### tty
显示当前终端的名字
### stty
显示或修改终端配置
### setterm
设置特定的终端属性
```sh
$ setterm -cursor off # 不显示光标
$ setterm -bold on # 将终端字体设置为粗体
```
### tset
显示或初始化终端设置
### setserial
设置或显示串口参数
```sh
# 来自于 /etc/pcmcia/serial 脚本:
IRQ=`setserial /dev/$DEVICE | sed -e 's/.*IRQ: //'`
setserial /dev/$DEVICE irq 0 ; setserial /dev/$DEVICE irq $IRQ
```
### getty,agetty
一个终端的初始化过程通常都是使用 getty 或 agetty 来建立, 这样才能让用户登录. 这些命令并不用在用户的 shell 脚本中. 它们的行为与 stty 很相似.
### mesg
设置与查看当前终端是否允许被其他终端写消息
### wall
向所有终端发送消息
> 备注:如果某个终端使用mesg禁用了写权限，那么就收不到wall发送的消息

## 信息和统计命令
### uname
打印系统说明到stdout
- -a 显示详细信息
- -s 显示系统类型
- -m 显示系统硬件架构
### arch
显示系统的硬件架构
### lastcomm
显示前一个命令
### lastlog
显示系统上所有用户最后登陆的时间
### lsof
显示当前所有打开的文件
### strace
追踪系统调用和信号，如strace df追踪df命令调用的所有文件
### ltrace
追踪库调用，如ltrace df最宗df命令所调用的库和函数
### nc
连接监听TCP和UDP端口
```sh
$ nc localhost.localdomain 25
220 localhost.localdomain ESMTP Sendmail 8.13.1/8.13.1;
 Thu, 31 Mar 2005 15:41:35 -0700
```
### free
显示内存和缓存的使用情况
### procinfo
从/proc pseudo-filesystem 中提取和显示所有信息和统计资料
```sh
$ procinfo | grep Bootup
Bootup: Wed Mar 21 15:15:50 2001 Load average: 0.04 0.21 0.34 3/47 6829
```
### lsdev
显示设备
### du
递归显示指定陌路下或当前目录下文件使用情况
### df
显示文件系统的使用情况
### dmesg
显示系统启动消息
### stat
显示一个或多个给定的文件的统计信息
### vmstat
显示虚拟内存的统计信息
### uptime
显示系统运行时间
### hostname
显示主机名
### hostid
显示主机的32位的16进制ID
### sar
sar(System Activity Reporter 系统活动报告)，是sysstat包的一部分
### readelf
显示指定elf格式的2进制文件的统计信息，是binutils包的一部分
### size
显示一个二进制可执行文件或归档文件每部分的尺寸
## 系统日志
### logger
附加一个用户产生的消息到系统日志中
```sh
logger Experiencing instability in network connection at 23:10, 05/21.
```
### logrotate
管理系统日志，在合适的时候轮换, 压缩, 删除, 和(或)e-mail这些日志。通常使用cron运行logrotate
## 任务控制
### ps
进程统计
### pgrep,pkill
ps与grep或者kill结合使用
### pstree
以树的形式列出当前执行的进程
### top
连续显示当前cpu使用率最高的进程
- -b 以文本方式显示，方便在脚本中分析
### nice
使用修改后的优先级来运行一个后台任务，优先级从 19(最低)到-20(最高)，只有 root 用户可以设置负的(比较高的)优先级.
### nohup
保持一个命令的运行，即使用户登出系统。如果在脚本中使用了nohup命令，最好结合wait命令，避免创建了一个孤儿进程或僵尸进程
### pidof
取得一个正在运行的进程的ID
### fuser
取得一个正在存取某些文件或目录的进程ID
- -k 将杀掉这些进程
- 获取正在读取某一端口的进程
- 在卸载U盘前可以使用这个fuser -um /dev/device_name查找并杀掉访问Upan的进程
### cron
管理任务调度
## 进程控制和boot
### init
所有进程的父进程，在kernel加载完成后调用
### telinit
init命令的符号链接，只能使用root身份调用，一般在系统维护或者修复文件系统时使用
### runlevel
显示当前和最后的运行级别
### halt,shutdown,reboot
设置系统关机的命令, 通常比电源关机的优先级高
### service
开启或停止一个系统服务
## 网络
### nmap
网络端口扫描器
### ifconfig
查看或配置网络接口
### netstat
显示当前网络的统计和信息，比如路由表和激活的链接
- -a 查看所有接口
- if 命令绝大多数情况都是在启动时候设置接口, 或者在重启的时候关闭它们
### iwconfig
配置无线网络
### ip
设置，修改，分析IP网络和链接的设备，是iproute2的一部分
```sh
$ ip link show
1: lo: <LOOPBACK,UP> mtu 16436 qdisc noqueue
     link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
 2: eth0: <BROADCAST,MULTICAST> mtu 1500 qdisc pfifo_fast qlen 1000
     link/ether 00:d0:59:ce:af:da brd ff:ff:ff:ff:ff:ff
 3: sit0: <NOARP> mtu 1480 qdisc noop
     link/sit 0.0.0.0 brd 0.0.0.0

$ ip route list
     169.254.0.0/16 dev lo  scope link
```
### route
显示内核路由表信息
### iptables
设置、维护和检查Linux内核的IP包过滤规则
### chkconfig
显示和管理在启动过程中开启的网络服务(这些服务都是从/etc/rc?.d 目录中开启的)
### tcpdump
分析和调试网络传输的工具，它使用的手段是把指定规则的包头显示出来
```sh
$ tcpdump ip host bozoville and caduceus # 显示主机 bozoville 和主机 caduceus 之间所有传输的 ip 包
```
## 文件系统
### mount
挂载文件系统
- /etc/fstab 列出所有可用的文件系统、分区和设备，以及一些选项，比如是否可以自动或手动挂载
- /etc/mtab 显示当前已经挂载的文件系统和分区(包括虚拟分区，如/proc)
```sh
# 检查 CD 镜像
# As root...

mkdir /mnt/cdtest  # Prepare a mount point, if not already there.

mount -r -t iso9660 -o loop cd-image.iso /mnt/cdtest   # Mount the image.
#                  "-o loop" option equivalent to "losetup /dev/loop0"
cd /mnt/cdtest     # Now, check the image.
ls -alR            # List the files in the directory tree there.
                   # And so forth.
```
### umount
卸载文件系统
### gnome-mount
替代mount和umount
### sync
强制将buffer上的数据写入硬盘(同步带有buffer的驱动器)
### losetup
配置和建立loopback设备(以普通文件虚拟块设备，将文件当做设备访问，比如虚拟光驱)
```sh
losetup /dev/loop0 a.iso           # Set it up as loopback device.
mount -o loop /dev/loop0 /mnt     # Mount it.
umount /mnt/
losetup -d /dev/loop0 # 分离关联设备
# Thanks, S.C.
```
### mkswap
创建交换分区。交换区域随后必须马上用 swapon 启用
### swapon，swapoff
启用/禁用交换分区，需要重启
### mke2fs
创建ext2文件系统，需root权限
### mkdosfs
创建FAT文件系统，需root权限
### tune2fs
调整ext2文件系统，可以用来修改文件系统参数，比如mount的最大数量，需root权限
### dumpe2fs
打印ext2文件系统信息，需root权限
### hdparm
列出或修改硬盘参数，需root权限
### fdisk
创建和修改分区表，需root权限
### fsck
检查和修复UNIX文件系统，默认是ext2
### e2fsck
检查ext2文件系统
### debugfs
ext2文件系统调试器，可以尝试恢复删除的文件(危险操作)
### badblocks
检查存储设备中的坏block(物理损坏)
### lsusb
列出所有USB设备
### usbmodules
列出所有连接USB设备的驱动模块
### lspci
列出所有PCI总线
### mkbootdisk
创建启动软盘, 启动盘可以唤醒系统
### mkisofs
创建CDR镜像文件
### chroot
修改根目录，默认的根目录是 /
### lockfile
锁定文件，是procmail包的一部分
### flock
锁定文件
### mknod
创建块或者字符设备文件
### MAKEDEV
创建设备文件的工具，需root权限
### tmpwatch
自动删除在指定时间内未被村去过的文件
## 备份
### dump,restore
dump 命令
### fdformat
对软盘进行低级格式化
## 系统资源
### ulimit
设置使用系统资源的上限，通常情况下ulimit 的值应该设置在/etc/profile 和(或)~/.bash_profile 中
- -f 限制文件的尺寸
- -t 限制coredump尺寸(coredump--核心转储，程序崩溃时将内存状态写入文件)
- -c ulimit -c 0就是不要 coredump
- -Hu 指定需要限制的用户进程，在/etc/profile中设置ulimit -Hu XX后，当 XX脚本超过预先设置的限制，就会自动终止这个脚本的运行
### quota
显示用户或组的磁盘配额
### setquota
从命令行中设置用户或组的磁盘配额
### umask
设定用户创建文件是权限的缺省掩码，用于设置创建文件的默认权限：比如, umask 022 将会使得新文件的权限最多为 755
### rdev
取得 root device, swap space, 或 video mode 的相关信息, 或者对它们进行修改
## 系统模块
### lsmod
列出所有安装的内核模块
### insmod
强制一个内核模块的安装(如果可能的话, 使用 modprobe 来代替),需root权限
### rmmod
强制卸载一个内核模块，需root权限
### modprobe
模块装载器, 一般情况下都是在启动脚本中自动调用，需root权限
### depmod
创建模块依赖文件, 一般都是在启动脚本中调用.
### modinfo
输出一个可装载模块的信息
## 其他
### env
- 指定脚本名为参数，使用设置过的或修改过的环境变量来运行一个程序或脚本
- 没指定参数，这个命令将会列出所有设置的环境变量
- 当不知道 shell 或解释器的路径的时候, 脚本的第一行(#!行)可以使用 env.
```sh
#! /usr/bin/env perl

print "This Perl script will run,\n";
print "even when I don't know where to find Perl.\n";

# Good for portable cross-platform scripts,
# where the Perl binaries may not be in the expected place.
# Thanks, S.C.
```
或者
```sh
#!/bin/env bash
# Queries the $PATH enviromental variable for the location of bash.
# Therefore ...
# This script will run where Bash is not in its usual place, in /bin.
```
### ldd
显示一个可执行文件的共享库的依赖关系
### watch
以指定的时间间隔来重复运行一个命令，默认是2秒
### strip
从可执行文件中去掉调试符号引用，这样做可以减小尺寸。一般用在Makefile中
### nm
列出未 strip 过的编译后的 2 进制文件的符号
### xrandr
控制屏幕的根窗口(root window)
### rdist
远程文件分布客户机程序: 在远端服务器上同步, 克隆, 或者备份一个文件系统
