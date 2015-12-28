#Linux启动
##系统启动流程
1. 加载 BIOS 的硬件资讯与进行自我测试，并依据配置取得第一个可启动的装置
2. 读取并运行第一个启动装置内 MBR 的 boot Loader (亦即是 grub, spfdisk 等程序)
3. 依据 boot loader 的配置加载 Kernel ，Kernel 会开始侦测硬件与加载驱动程序
4. 在硬件驱动成功后，Kernel 会主动呼叫 init 程序，而 init 会取得 run-level 资讯
5. init 运行 /etc/rc.d/rc.sysinit 文件来准备软件运行的作业环境 (如网络、时区等)
6. init 运行 run-level 的各个服务之启动 (script 方式)
7. init 运行 /etc/rc.d/rc.local 文件
8. init 运行终端机模拟程序 mingetty 来启动 login 程序，最后就等待使用者登陆
##BIOS->MBR
BIOS 是透过硬件的 INT 13 中断功能来读取第一块硬盘的MBR，这样就找到了第一块硬盘上的boot loader
##Boot Loader
- MBR的boot loader只有一个，负责找到目标系统上的boot loader
- 只有目标系统的boot loader才能加载属于自己的kernel
- windows的boot loader默认不能识别linux的boot loader
- grub可以识别windows的boot loader
##加载kernel并检测硬件
- 系统根据Boot Loader的配置，在/boot路径下找到并加载kernel文件， 然后开始测试并加载核心模块和驱动
- 核心模块和驱动位于/lib/modules，且/lib目录和根目录一般在同一个分区，所以启动过程中kernel必须要挂载根目录
- 当根目录位于特殊的存储介质上(U盘, SATA, SCSI)，或者根目录使用特殊的文件系统(LVM, RAID)导致kernel无法挂载根目录，kernel通过挂在虚拟文件系统(Initial
   RAM Disk)到根目录来实现启动
   >initrd一般位于/boot/initrd，所以一般用独立的分区挂载/boot

- initrd加载进内存后，kernel将其挂载为根目录，然后借此加载驱动程序，最终释放initrd，并在实际的文件系统上挂载根目录
##init程序
init是内核加载后执行的第一个程序，主要功能是准备软件运行的环境，包括系统的主机名称、网络设置、语系设置、文件系统、启动服务等，所有这些都通过读取/etc/inittab设置来完成
###inittab的内容
####格式
```
[配置项目]:[run level]:[init 的动作行为]:[命令项目]
```
####配置项目
最多四个字节，代表 init 的主要工作项目，只是一个简单的代表说明
####run level
该项目在哪些 run level 底下进行的意思。如果是 35 则代表 runlevel 3 与 5 都会运行
- 0 - halt (系统直接关机)
- 1 - single user mode (单人维护模式，用在系统出问题时的维护)
- 2 - Multi-user, without NFS (类似底下的 runlevel 3，但无 NFS 服务)
- 3 - Full multi-user mode (完整含有网络功能的纯文字模式)
- 4 - unused (系统保留功能)
- 5 - X11 (与 runlevel 3 类似，但加载使用 X Window)
- 6 - reboot (重新启动)
####run level 切换
```sh
# runlevel
N 5
# 左边代表前一个 runlevel ，右边代表目前的 runlevel。
# 由於之前并没有切换过 runlevel ，因此前一个 runlevel 不存在 (N)
```
```sh
# init N
# N为0到6
```
####init 的动作行为
- initdefault 代表默认的 run level 配置值
- sysinit 代表系统初始化的动作项目
- ctrlaltdel 代表 [ctrl]+[alt]+[del] 三个按键是否可以重新启动的配置
- wait 代表后面栏位配置的命令项目必须要运行完毕才能继续底下其他的动作
- respawn 代表后面栏位的命令可以无限制的再生 (重新启动)。举例来说， tty1 的 mingetty 产生的可登陆画面， 在你注销而结束后，系统会再开一个新的可登陆画面等待下一个登陆。
###init 的处理流程
####/etc/inittab
```sh
id:5:initdefault:- - - -  #<==默认的 runlevel 配置, 此 runlevel 为 5
si::sysinit:/etc/rc.d/rc.sysinit  #<==准备系统软件运行的环境的脚本运行档
# 7 个不同 run level 的，需要启动的服务的 scripts 放置路径：
l0:0:wait:/etc/rc.d/rc 0- #<==runlevel 0 在 /etc/rc.d/rc0.d/
l1:1:wait:/etc/rc.d/rc 1- #<==runlevel 1 在 /etc/rc.d/rc1.d/
l2:2:wait:/etc/rc.d/rc 2- #<==runlevel 2 在 /etc/rc.d/rc2.d/
l3:3:wait:/etc/rc.d/rc 3- #<==runlevel 3 在 /etc/rc.d/rc3.d/
l4:4:wait:/etc/rc.d/rc 4- #<==runlevel 4 在 /etc/rc.d/rc4.d/
l5:5:wait:/etc/rc.d/rc 5- #<==runlevel 5 在 /etc/rc.d/rc5.d/
l6:6:wait:/etc/rc.d/rc 6- #<==runlevel 6 在 /etc/rc.d/rc6.d/
# 是否允许按下 [ctrl]+[alt]+[del] 就重新启动的配置项目：
ca::ctrlaltdel:/sbin/shutdown -t3 -r now
# 底下两个配置则是关於不断电系统的 (UPS)，一个是没电力时的关机，一个是复电的处理
pf::powerfail:/sbin/shutdown -f -h +2 "Power Failure; System Shutting Down"
pr:12345:powerokwait:/sbin/shutdown -c "Power Restored; Shutdown Cancelled"
1:2345:respawn:/sbin/mingetty tty1  #<==其实 tty1~tty6 是由底下这六行决定的。
2:2345:respawn:/sbin/mingetty tty2
3:2345:respawn:/sbin/mingetty tty3
4:2345:respawn:/sbin/mingetty tty4
5:2345:respawn:/sbin/mingetty tty5
6:2345:respawn:/sbin/mingetty tty6
x:5:respawn:/etc/X11/prefdm -nodaemon #<==X window 则是这行决定的！
```
1. 先取得 runlevel 亦即默认运行等级的相关等级 (默认为5号)；
2. 使用 /etc/rc.d/rc.sysinit 进行系统初始化
3. 由於 runlevel 是 5 ，因此只进行『l5:5:wait:/etc/rc.d/rc 5』，其他行则略过
4. 配置好 [ctrl]+[alt]+[del] 这组的组合键功能
5. 配置不断电系统的 pf, pr 两种机制；
6. 启动 mingetty 的六个终端机 (tty1 ~ tty6)
7. 最终以 /etc/X11/perfdm -nodaemon 启动图形介面
####/etc/rc.d/rc.sysinit
- 取得网络环境与主机类型： 读取网络配置档 /etc/sysconfig/network ，取得主机名称与默认通讯闸 (gateway) 等网络环境。
- 测试与挂载内存装置 /proc 及 U盘 装置 /sys： 除挂载内存装置 /proc 之外，还会主动侦测系统上是否具有 usb 的装置， 若有则会主动加载 usb 的驱动程序，并且尝试挂载 usb 的文件系统。
- 决定是否启动 SELinux ： SELinux 检测， 并且检测是否需要帮所有的文件重新编写标准的 SELinux 类型 (auto relabel)。
- 启动系统的随机数生成器： 随机数生成器可以帮助系统进行一些口令加密演算的功能，在此需要启动两次随机数生成器。
- 配置终端机 (console) 字形：
- 配置显示於启动过程中的欢迎画面 (text banner)；
- 配置系统时间 (clock) 与时区配置：需读入 /etc/sysconfig/clock 配置值
- 周边设备的侦测与 Plug and Play (PnP) 参数的测试： 根据核心在启动时侦测的结果 (/proc/sys/kernel/modprobe ) 开始进行 ide / scsi / 网络 / 音效 等周边设备的侦测，以及利用以加载的核心模块进行 PnP 装置的参数测试。
- 使用者自定义模块的加载 使用者可以在 /etc/sysconfig/modules/.modules 加入自定义的模块，则此时会被加载到系统当中
- 加载核心的相关配置： 系统会主动去读取 /etc/sysctl.conf 这个文件的配置值，使核心功能成为我们想要的样子。
- 配置主机名称与初始化电源管理模块 (ACPI)
- 初始化软件磁盘阵列：主要是透过 /etc/mdadm.conf 来配置好的。
- 初始化 LVM 的文件系统功能
- 以 fsck 检验硬盘文件系统：会进行 filesystem check
- 进行硬盘配额 quota 的转换 (非必要)：
- 重新以可读写模式挂载系统硬盘：
- 启动 quota 功能：所以我们不需要自定义 quotaon 的动作
- 启动系统虚拟随机数生成器 (pseudo-random)：
- 清除启动过程当中的缓存文件：
- 将启动相关资讯加载 /var/log/dmesg 文件中
####/etc/rc.d/rc N
通过执行/etc/rc.d/rc N下的脚本，进入到/etc/init.d下找到相应的服务脚本，然后进行start(Sxx)或stop(Kxx)操作
```sh
# ll /etc/rc5.d/
lrwxrwxrwx 1 root root 16 Sep  4  2008 K02dhcdbd -> ../init.d/dhcdbd
....(中间省略)....
lrwxrwxrwx 1 root root 14 Sep  4  2008 K91capi -> ../init.d/capi
lrwxrwxrwx 1 root root 23 Sep  4  2008 S00microcode_ctl -> ../init.d/microcode_ctl
lrwxrwxrwx 1 root root 22 Sep  4  2008 S02lvm2-monitor -> ../init.d/lvm2-monitor
....(中间省略)....
lrwxrwxrwx 1 root root 17 Sep  4  2008 S10network -> ../init.d/network
....(中间省略)....
lrwxrwxrwx 1 root root 11 Sep  4  2008 S99local -> ../rc.local
lrwxrwxrwx 1 root root 16 Sep  4  2008 S99smartd -> ../init.d/smartd
....(底下省略)....
```
####/etc/rc.d/rc.local
配置自定义启动程序
####/etc/modprobe.conf
配置/etc/sysconfig/modules中的自定义模块
####/etc/sysconfig/*
#####authconfig
这个文件主要在规范使用者的身份认证的机制，包括是否使用本机的 /etc/passwd, /etc/shadow 等， 以及 /etc/shadow 口令记录使用何种加密算法，还有是否使用外部口令服务器提供的帐号验证 (NIS, LDAP) 等
#####clock
此文件在配置 Linux 主机的时区
#####i18n
i18n 在配置一些语系的使用方面
#####keyboard & mouse
keyboard 与 mouse 就是在配置键盘与鼠标的形式
#####network
network 可以配置是否要启动网络，以及配置主机名称还有通讯闸 (GATEWAY)
#####network-scripts
配置网卡信息
