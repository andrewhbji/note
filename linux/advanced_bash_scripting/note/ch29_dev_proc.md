[TOC]
# /dev和/proc
## 29.1 /dev
### 物理设备
/dev目录包含物理设备的条目, 这些设备可能会以硬件的形式出现, 也可能不会。挂载文件系统的硬盘分区，在/dev目录中都有对应的条目
### 环回设备
/dev目录也包含环回设备, 比如/dev/loop0. 一个环回设备就是一种机制, 可以让一般文件访问起来就像块设备那样.
### 伪设备
/dev中还有少量的伪设备用于其它特殊目的, 比如/dev/null, /dev/zero, /dev/urandom, /dev/sda1(串口硬盘分区被当做伪设备？), /dev/udp, 和/dev/tcp
>备注：
> 1. 串口硬盘分区被当做伪设备？
> 2. 当在/dev/tcp/$host/$port伪设备文件上执行一个命令的时候, Bash会打开一个TCP连接, 也就是打开相关的socket.

```sh
bash$ exec 5<>/dev/tcp/www.net.cn/80
bash$ echo -e "GET / HTTP/1.0\n" >&5
bash$ cat <&5
```

## 29.2 /proc
/proc目录实际上是一个伪文件系统. /proc目录中的文件用来映射当前运行的系统
### 系统信息
- /proc/devices 当前所有硬件
- /proc/interrupts 中断信息
- /proc/partitions 分区信息
- /proc/loadavg 负载均衡信息
- /proc/apm 高级电源管理信息
- /proc/acpi/battery/BAT0/info 电池信息
- /proc/meminfo 内存信息
- /proc/filesystems 文件系统
- /proc/version 内核版本
- /proc/cpuinfo CPU信息
- /proc/bus/usb/devices USB设备信息
### 进程信息
/proc目录下包含有许多以不同以进程ID命名的子目录，保存对应进程的可用信息
- 文件stat和status保存着进程运行时的各项统计信息
- 文件cmdline保存着进程被调用时的命令行参数
- 文件exe是一个符号链接，指向这个运行进程的完整路径
