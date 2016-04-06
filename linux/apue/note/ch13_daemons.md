[TOC]
# 第13章 守护进程
守护进程 UNIX 环境中用于执行日常事物活动、提供服务的进程，守护进程由 init 进程启动，没有控制终端。

一般使用 ps -axj 查看守护进程
- -a 选项显示所有进程
- -x 选项查看没有控制终端的进程
- -j 显示和进程相关的信息

守护进程一般以 ***d 结尾

## 守护进程编程规则
1. 调用 umask 函数设置重置文件权限掩码，一般设置为0
>备注：因为守护进程是fork产生的，会继承父进程的文件权限掩码

2. 调用 fork，然后使父进程 exit
>备注：这样做实现了下面几点
>- 如果守护进程使通过 shell 命令启动的，那么父进程终止会让 shell 认为这条命令已经执行完毕，这样做使其能让 init 托管
>- 虽然子进程继承了父进程的pgid，却有了新的pid

3. 子进程调用 setsid 创建一个会话
>备注：
>- 这样子进程就成了新会话的首进程，同时创建一个进程组并成为组长，且子进程没有控制终端
>- 在基于System V的系统中，有人建议再fork一次，然后终止父进程，继续使用子进程中的守护进程。这样就保证了该守护进程不是会话首进程，按照 System V的规则，可以防止其获取控制终端

4. 使用 chdir 将当前工作目录切换到 /
>备注：这样避免挂载失败

5. 关闭不需要的文件描述符
>备注：可以使用 open_max 函数 或 getrlimit 函数来判断最高的文件描述符值，然后循环关闭

6. 某些守护进程在 fd0 、fd1 、fd2 打开 /dev/null 来确保关闭所有输入输出

## 日志
UNIX 日志使用 syslog架构，系统服务 syslogd 通过侦听 /dev/log socket、 /dev/klog socket 和 UDP514 端口收集信息，并将日志记录在指定文件
>备注：/dev/log用于接收本地用户进程的日志信息，UDP514端口接收网络上的日志信息，/dev/klog则是监听内核的日志信息。

日志产生的途径：
- 内核程序调用 log 函数，任何一个用户进程都可以打开读取 /dev/klog 读取内核日志
- 守护进程调用 syslog 函数来生成日志信息，并存储在 /dev/log
- 任何进程都可以向 UDP514 端口发送日志信息

### syslog 函数
```
#include <syslog.h>
void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);
int setlogmask(int maskpri);
返回值：返回调用进程之前设置的优先级设置掩码
```
#### openlog
openlog 函数用于初始化日志，创建到syslogd的连接
- ident 设置日志的标识符
- option 用来控制 openlog 和后续调用 syslog 的行为，可用取值如下：
|  取值  |  描述  |
|:--|:--|
|  LOG_CONS  |  若不能通过 socket 将日志信息(传输 UNIX Domain数据报)送到 Syslogd，则直接写入控制台  |
|  LOG_NDELAY  |  立即打开连接到 syslogd 的 socket(传输UNIX Domain数据报)  |
|  LOG_NOWAIT  |  不要等待在将消息记录日志过程中可能已经创建的子进程，因为在syslog调用wait时，应用程序可能已获得了子进程的状态。这种处理阻止了与捕捉SIGCHLD信号的应用程序之前产生的冲突(GNU C不会创建子进程，所以对Linux没有影响)  |
|  LOG_ODELAY  | 在第一条消息被记录之前延迟打开到 syslogd 的socket  |
|  LOG_PERROR  | 除将日志消息发送给 syslogd 意外，还将其写入到stderr  |
|  LOG_PID  | 日志中包含PID  |

- facility 指定日志的类型。不同类型的消息会以不同的方式处理。如果不掉用 openlog，或者 openlog 的 facility 参数为 0 ， 调用 syslog 函数时，  facility 将和 priority 参数一起指定日志的严重界别。 facility 取值如下：
|  取值  |  描述  |
|:--|:--|
|  LOG_AUTH  |  审计设施  |
|  LOG_AUTHPRIV  |  授权程序：login、su、gettty 等与 LOG_AUTH 相同，但写日志文件时具有权限限制  |
|  LOG_CONSOLE  |  消息写入/dev/console  |
|  LOG_CRON  |  cron 和 at  |
|  LOG_DAEMON  |  inetd、routed等系统服务  |
|  LOG_FTP  | ftpd系统服务  |
|  LOG_KERN  |  内核消息   |
|  LOG_LOCAL0 到 LOG_LOCAL7  |  系统保留   |
|  LOG_LPR  |  行式打印机系统： lpd、lpc等   |
|  LOG_MAIL  |  邮件系统  |
|  LOG_NEWS  |  Usenet网络新闻系统  |
|  LOG_SYSLOG  |  syslogd守护进程本身  |
|  LOG_USER (默认)  |  来自其他用户进程的消息  |
|  LOG_UUCP  |  syslogd守护进程不本身  |
|  LOG_NTP  |  网络时间协议系统  |
|  LOG_SECURITY  |  安全子系统  |
 
#### syslog 函数
syslog 用于产生一条日志
- priority 用于指定日志的级别，取值如下：
|  取值  |  描述  |
|:--|:--|
|  LOG_EMERG  |  紧急  |
|  LOG_ALERT  |  必须立即修复  |
|  LOG_CRIT  |  严重错误  |
|  LOG_ERR  |  错误  |
|  LOG_WARNING  |  警告  |
|  LOG_NOTICE  |  正常，但是值得注意  |
|  LOG_INFO  |  一般信息  |
|  LOG_DEBUG  |  调试消息  |

- format 以及后面的参数列表和 vsprintf 基本一致

#### setlogmask 函数
setlogmask 函数用于设置日志级别掩码，用来指定一部分级别的日志不被记录

#### closelog 函数
closelog 函数是可选的，程序结束后自动关闭与syslogd的连接

## 单实例守护进程
如果守护进程需要访问设备，且设备驱动已经限制了对设备节点的多次访问，同一时间就只能有一个进程访问该设备，这样的守护进程就必须是单实例(单进程)的。

守护进程在实现是一般都是单进程的，如果某个设备没有限制对其多次访问，就需要使用记录锁机制保证守护进程以单进程的形式运行

## 守护进程的惯例
- 如果守护进程使用锁文件，那么该文件通常存储在/var/run目录中。不过需要注意，守护进程需要超级用户权限才能创建文件。锁文件名字一般是name.pid
- 如果守护进程支持配置文件，则配置文件一般存储在/etc目录中。
- 守护进程可以通过命令行启动，但是通常是使用init脚本启动的。
- 如果一个守护进程有一个配置文件，在启动的时候会读取该文件，但是在此之后一般就不会再查看它。如果管理员更改了配置文件，那么该守护进程可能需要重新启动，后来在信号机制中加入了SIGHUP信号的捕捉，让守护进程接收到信号后重新读取配置文件。

## 客户进程-服务器进程模型
C/S进程模型在Unix环境中非常常见，守护进程通常就是服务器进程，然后等待客户进程与其联系，处理客户进程的请求。为了保证请求的高效处理，服务器进程中调用fork然后exec另一个程序来提供服务是非常常见的。这些服务器进程通常管理着多种资源。而为了保证文件描述符不被滥用，所以需要对所有被执行程序不需要的文件描述符设置成执行时关闭（close-on-exec）。