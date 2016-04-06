[TOC]
# 第9章 进程关系

## 进程组
进程组是被共同作业关联起来的一组进程的集合，主要用于相关进程之间的信号传递和作业控制

所有活动的进程都是某个进程组的成员，进程组ID

### getgprp getpgid
getpgrp函数是返回当前进程的进程组ID, getpgid返回指定pid的进程组ID
```c
#include <unistd.h>
pid_t getpgrp(void);
pid_t getpgid(pid_t pid);
成功返回进程组ID，出错返回-1
```
- pid 为 0，则返回当前进程的进程组ID

### setpgid
加入一个进程组或创建进程组
```c
#include <unistd.h>
int setpgid(pid_t pid, pid_t pgid);
返回值：成功返回0，出错返回-1
```
setpgid函数将pid进程的进程组ID设置为pgid，如果pid为0，则使用调用者的进程ID，如果pgid为0，则由pid指定的进程ID作为进程组ID。

## 会话
会话是一系列进程组的集合，用户登录就是一个会话的开始，登录之后，用户会得到一个与终端相关联的进程，该进程就是该会话的leader，会话的id就是该进程的id。

### setsid
setsid创建会话会作三件事
- 调用setsid函数的进程是该会话的首进程（session leader），也是新会话的唯一进程
- session leader 成为一个新进程组的组长进程，新进程组id是该进程id
- session leader 是没有控制终端的,如果该进程原来是有一个控制终端的，但是这种联系也会被打断.因此，在新建一个session的时候需要对输入输出进行重定向
```c
#include <unistd.h>
pid_t setsid(void);
返回值：成功返回进程组ID，出错返回-1
```

## 控制终端
控制终端就是和session结合的终端设备，另外关于会话和进程组
- 会话有控制终端，只要是能实现终端功能的设备都行
- 和控制终端建立连接的会话首进程是控制进程
- 会话中的进程组可以分为前台进程组、后台进程组
- 存在控制终端则必定存在前台进程组
>备注：
> 1. 控制进程就是带有控制终端的session leader
> 2. SSH 是比较典型的 session的例子，前台进程组在SSH断线后就会自动终止，而后台进程组则不会终止

## tcgetpgrp、tcsetpgrp tcgetsid
tcgetpgrp 根据终端设备的文件描述符返回前台进程组ID

tcsetpgrp 当进程有一个控制终端时，该进程可以调用tcsetpgrp，将pgid指定的进程组设置为前台进程组(该进程组必须是由session leader派生出来的进程组)

tcgetsid 根据终端设备的文件描述符获得session leader的 ID
```c
#include <unistd.h>
pid_t tcgetpgrp(int fd);
返回值：成功返回前台进程组ID，出错返回-1
int tcsetpgrp(int fd, pid_t pgid);
返回值：成功返回0，出错返回-1
#include <termios.h>
pid_t tcgetsid(int fd);
返回值：成功返回session leader的 ID，出错返回-1
```

## 作业控制
作业控制就是在一个终端下进行多任务处理时，控制哪个任务可以访问终端设备，哪个任务在后台运行

终端设备通过三个特殊字符影响前台进程组
- Ctrl+C 产生中断信号 SIGINT
- Ctrl+\ 产生退出信号 SIGQUIT
- Ctrl+Z 产生挂起 SIGTSTP
