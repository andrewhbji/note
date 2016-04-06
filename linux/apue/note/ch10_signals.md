[TOC]
# 第10章 信号

## 信号概念
信号用于进程间交互，比如处理异步事件

UNIX提供了了完善的信号机制，开发者只需要让进程注册函数来处理信号

进程收到信号后，可以选择3种处理方式：
1. 忽略信号
2. 捕捉信号
3. 执行系统默认行为

### 常用信号
| 信号名     | 默认行为     | 描述     |
|:--|:--|:--|
| SIGHUP  | 终止进程  | 用户终端连接(正常或非正常)结束时发出,通常是在终端的控制进程结束时, 通知同一session内的各个作业,这时它们与控制终端不再关联  |
| SIGINT  | 终止进程  | 程序中止(interrupt)信号,通常是从终端发出中断指令如ctrl+c或delete键 |
| SIGQUIT  | 终止进程+创建core image  | 由QUIT字符(通常是Ctrl+\)产生，类似于一个程序错误信号  |
| SIGILL  | 终止进程+创建core image  | 执行非法指令时产生  ，通常是因为可执行文件本身出现错误,或者试图执行数据段. 堆栈溢出时也有可能产生这个信号     |
| SIGTRAP  | 终止进程+创建core image  | 跟踪陷阱信号，由断点指令或其它trap指令产生.由debugger使用.  |
| SIGABRT  | 终止进程+创建core image  | 调用abort时产生的信号，将会使进程非正常结束  |
| SIGEMT  | 终止进程+创建core image  | 实时硬件发生错误时产生的信号  |
| SIGFPE  | 终止进程+创建core image  | 在发生致命的算术运算错误时发出.不仅包括浮点运算错误,还包括溢出及除数为0等其它所有的算术的错误.  |
| SIGKILL  | 终止进程  | 杀死进程时产生  |
| SIGBUS  | 终止进程+创建core image  | 系统总线错误时产生的信号，非法地址,包括内存地址对齐(alignment)出错.eg: 访问一个四个字长的整数, 但其地址不是4的倍数.  |
| SIGSEGV  | 终止进程+创建core image  | 试图访问未分配给自己的内存,或试图往没有写权限的内存地址写数据，非法使用内存地址信号  |
| SIGSYS  | 终止进程+创建core image  | 系统调用异常  |
| SIGPIPE  | 终止进程  | 管道出现异常  |
| SIGALRM  | 终止进程  | 时钟定时信号,由alarm函数设定的时间终止时产生的信号  |
| SIGTERM  | 终止进程  | 程序终止信号  |
| SIGURG  | 忽略信号  | I/O有紧急数据到达当前进程  |
| SIGSTOP  | 停止进程  | 进程停止时产生  |
| SIGTSTP  | 停止进程  | 终端停止时产生  |
| SIGCONT  | 忽略信号  | 进程继续时产生  |
| SIGCHLD  | 忽略信号  | 子进程终止或中断时产生  |
| SIGTTIN  | 停止进程  | 后台进程读控制终端时产生(后台进程不允许读控制终端)  |
| SIGTTOU  | 停止进程  | 后台进程写控制终端时产生(后台进程不允许写控制终端)  |
| SIGIO  | 终止进程/忽略信号  | 文件描述符准备就绪时产生,可以开始进行输入/输出操作  |
| SIGXCPU  | 终止进程+创建core image  | 超过CPU时间资源限制时产生  |
| SIGXFSZ  | 终止进程+创建core image  | 超过文件大小资源限制时产生  |
| SIGVTALRM  | 终止进程  | 虚拟时钟(进程CPU时间)超时时产生  |
| SIGPROF  | 终止进程  | profile(进程CPU时间以及系统调用时间)时钟超时时产生  |
| SIGWINCH  | 忽略信号  | 窗口大小改变时发出  |
| SIGPOLL  | 终止进程  | 用于轮询  |
| SIGPWR  | 终止进程/忽略信号  | 关机信号  |
| SIGSTKFLT  | 终止进程  | 栈溢出时产生  |
| SIGUSR1  | 终止进程  | 预留信号  |
| SIGUSR2  | 终止进程  | 预留信号  |

## signal 函数
注册信号处理函数，当进程收到信号sig时，调用func函数处理
```c
#include <signal.h>
void (*signal(int sig, void (*func)(int)))(int);
返回值：成功返回以前注册的func函数，出错则返回SIG_ERR
```
- sig 指定要处理的信号
- \*func 可以是函数指针，该函数参数为int，无返回值,当进程收到信号时，调用该函数
- \*func 也可以时常量SIG_IGN SIG_DFL,SIG_IGN表示内核忽略此信号，常量SIG_DFL表示执行内核默认动作

## 不可靠的信号
不可靠在这里指的是：信号可能丢失，一个信号出现了，但进程却可能收不到。对于Linux 信号值小于SIGRTMIN的信号都是不可靠信号，这里的不可靠是指不支持信号队列：比如，短时间内出现20个SIGRTMIN和SIGUSR1信号，进程可以接收处理全部的SIGRTMIN信号，但是只能接收处理1个SIGUSR1信号

## 中断的系统调用
对于正在执行低速系统调用的时候，收到信号，那么进程就会捕捉到这个信号，中断这个低速的系统调用，并且把errno设置为EINTR表示被信号中断了

## 可重入函数
信号处理程序会阻塞正在执行的函数，大部分函数在被阻塞的时候，不能在信号处理函数中被重新调用(不可重入的原因是函数内部使用了全局静态数据结构，每一个进程调用的时候，会写乱这个全局数据结构，或者api内部调用了malloc或者free在修改链表)，而一部分可重入函数函数(Single UNIX Specification声明)没有这个限制

### 可重入函数包括
abort accept access aio_error aio_return aio_suspend alarm bind cfgetispeed cfgetospeed cfsetispeed cfsetospeed chdir chmod chown clock_gettime close connect creat dup dup2 execl execle execv execve \_Exit \_exit

faccessat fchmod fchmodat fchown fchownat fcntl fdatasync fexecve fork fstat fstatat fsync ftruncate futimens getegid geteuid getgid getgroups getpeername getpgrp getpid getppid getsockname getsockopt getuid kill link

linkat listen lseek lstat mkdir mkdirat mkfifo mkfifoat mknod mknodat open openat pause pipe poll posix_trace_event pselect raise read readlink readlinkat recv recvfrom recvmsg rename renameat rmdir

select sem_post send sendmsg sendto setgid setpgid setsid setsockopt setuid shutdown sigaction sigaddset sigdelset sigemptyset sigfillset sigismember signal sigpause sigpending sigprocmask sigqueue sigset sigsuspend sleep sockatmark socket

socketpair stat symlink symlinkat tcdrain tcflow tcflush tcgetattr tcgetpgrp tcsendbreak tcsetattr tcsetpgrp time timer_getoverrun timer_gettime timer_settime times umask uname unlink unlinkat utime utimensat utimes wait waitpid write
>备注：这些函数也被称为是异步信号安全的

## SIGCLD信号
SIGCLD和SIGCHLD是两个很相似的信号，SIGCLD是SystemV的一个信号名字，而SIGCHLD是BSD信号，但是POSIX.1标准使用了BSD的SIGCHLD信号名称

SIGCHLD信号是很普通的意思，就是子进程状态改变就会产生这个信号，父进程则是调用wait函数查看子进程的状态

SIGCLD信号则不同:
- 如果进程明确配置SIGCLD信号为SIG_IGN，则调用进程的子进程不产生僵尸进程(调用进程结束，子进程也会结束(调用进程无需清理子进程))，子进程结束的时候丢弃其退出状态
- SIGCLD被设置为捕捉，则内核就会立刻检查是否有子进程准备好了等待

## 可靠信号术语和语义
信号是在事件(一般是硬件异常事件，软件条件触发事件)发生时，内核发送给进程的，或是通过内核转发终端生成和调用kill函数生成的信号。

信号从产生到传递给进程，这期间信号是待处理的(pending)的

进程可以通过设置信号掩码来阻塞(block)信号，当一个信号被设置为阻塞，无论该信号的行为时系统默认还是捕捉该信号，被设置为阻塞进程都忽略该信号，直到进程将该信号解除阻塞

POSIX.1没有定义可靠信号的传递顺序，猜测是以队列的方式传递

## 发送信号
kill属于系统函数库，给指定的进程发送信号

raise属于ISO C函数库，给自己发送信号

发送信号需要遵循UNIX的进程权限

```c
#include <signal.h>
int kill(pid_t pid, int sig);
int raise(int sig);
```
raise(sig) 等价于 kill(getpid(),signo)

### pid
- pid > 0 信号发送给指定pid的进程
- pid == 0 信号发送给进程组
- pid == -1 信号发送给所有进程
- pid < -1 信号发送给pid绝对值的进程

### 发送权限
- root 可以给任何进程发送信号
- 具有相同的ruid/rgid、euid/egid 和 saved set-user/group-ID 的进程可以相互收发信号
- SIGNCONT 信号可以发送给session内的任意进程(无权限限制)

## alarm和pause函数
pause使进程休眠，alarm用来设置休眠的时间
- alarm函数就是设置一个定时器，到期后会产生一个 SIGALRM 信号，如果忽略或者系统默认动作，就是进程终止，当然，一般情况下，进程都捕捉该信号。
- pause函数会强制进程暂停直到从 kill 函数或者 setitimer 函数收到一个信号
```c
#include <unistd.h>
unsigned int alarm(unsigned int seconds);
返回值：返回之前设置的seconds，如果之前没有设置，返回0
int pause(void);
返回值：出错返回-1
```

## 信号集
为了能够表示多个信号，系统提供了信号集的数据类型。在通常的开发中，经常会使用到二进制位来表示状态，二进制的每一位代表一种信号，但是实际上，由于信号的编号肯定会超过一个整形量的位数，所以一般都不是用一个整形量表示信号集。POSIX.1定义了数据类型sigset_t用以表示一个信号集
```c
typedef struct
  {
    unsigned long int __val[_SIGSET_NWORDS];
  } __sigset_t;
typedef __sigset_t sigset_t;
```

### 信号集函数
用来设置信号集，这些函数主要与信号阻塞相关函数配合使用
```c
#include <signal.h>
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
返回值：成功返回0，出错返回-1
int sigismember(const sigset_t *set, int signo);
返回值：是则返回1，否则返回0，出错返回-1
```
sigemptyset函数初始化set指向的信号集，清除所有信号，sigfillset初始化set指向的信号集，被设置为包含所有信号，在使用之前，sigemptyset或者sigfillset必须被调用。sigaddset和sigdelset则是添加删除一个信号，sigismember函数则返回是否一个指定的signo信号被包含在这个信号集中

## sigprocmask函数
为信号集设置信号掩码，即把指定信号集的信号处理设置为阻塞(即不能传递)
```c
#include <signal.h>
int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oldset);
返回值：成功返回0，出错返回-1
```
如果set不是null，sigprocmask函数的行为依赖how参数
- SIG_BLOCK 或者 0，进程新的信号掩码是当前信号掩码和set指向信号集的并集运算，即将set添加到当前信号集
- SIG_UNBLOCK 或者 1，新的信号掩码是当前信号掩码和set指向信号集的补集的交集，即将set从当前信号集移除
- SIG_SETMASK 或者 2，将当前信号集替换为set

如果oset不是null，那么当前信号集将会被赋值给oset，如果set参数为null，则不改变信号集，how参数也没有意义。

## sigpending 函数
sigpending函数返回当前进程阻塞不能传递信号的信号集
```c
#include <signal.h>
int sigpending(sigset_t *set);
返回值：成功返回0，出错返回-1
```

## sigaction 函数
sigaction函数用于查询或修改与指定信号相关联的处理动作
```c
#include <signal.h>
int sigaction(int signum, const struct sigaction *restrict act, struct sigaction *restrict oldact);
```
- sigaction() 为 signum 指定的信号设置处理函数，signum可以指定SIGKILL和SIGSTOP以外的所有信号
- 如果act指针非空，则 signum 信号的处理函数由act参数指定
- 如果oldact非空，则记录 signum 信号之前的处理函数

### sigaction 结构体
```c
struct sigaction {
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t   sa_mask;
    int    sa_flags;
    void     (*sa_restorer)(void);
};
```
- sa_handler 是函数指针，记录默认处理函数，参数为(int signo)，无返回值
- sa_sigaction 是函数指针，如果sa_flags 设置为 SA_SIGINFO，使用sa_sigaction 记录处理函数，参数为 (int signo, siginfo_t \*info, void \*context), 无返回值
- sa_mask 设置信号集，用于临时设置信号掩码，当信号处理函数被调用后，信号掩码将恢复先前设置
- sa_flags 用来设置信号处理的其他相关操作，XSI标准默认是SA_INTERRUPT:
  - SA_INTERRUPT：系统调用中断后不会重启
  - SA_NOCLDSTOP：如果 signo 是 SIGCHLD，子进程停止时不会产生 SIGCHLD，但是，子进程终止时，仍然会产生 SIGCHLD
  - SA_NOCLDWAIT：如果 signo 是 SIGCHLD，SA_NOCLDWAIT 会阻止子进程成为僵尸进程
  - SA_NODEFER：当信号被捕获，信号不会被系统自动阻塞，直到信号捕获函数执行(除非信号包含在sa_mask)中
  - SA_ONSTACK：信号可以可以在替换的信号栈上发送给进程(信号栈使用int sigaltstack(const stack_t \*ss, stack_t \*oss);创建)
  - SA_RESETHAND：如果signo不是SIGILL或SIGTRAP，信号处理函数被重置为SIG_DFL,清空SA_SIGINFO设置
  - SA_RESTART：被信号中止的系统调用会自动重启
  - SA_SIGINFO：使用 sa_sigaction 记录处理函数
- context 参数为 ucontext_t 结构体，用于标识信号传递时的进程context(通常不指定第三个参数)

### siginfo 结构体
siginfo 结构体包含和信号生成原因相关的信息
```c
siginfo_t {
    int      si_signo;    /* Signal number */
    int      si_errno;    /* An errno value */
    int      si_code;     /* Signal code */
    int      si_trapno;   /* Trap number that caused hardware-generated signal (unused on most architectures) */
    pid_t    si_pid;      /* Sending process ID */
    uid_t    si_uid;      /* Real user ID of sending process */
    int      si_status;   /* Exit value or signal */
    clock_t  si_utime;    /* User time consumed */
    clock_t  si_stime;    /* System time consumed */
    sigval_t si_value;    /* Signal value */
    int      si_int;      /* POSIX.1b signal */
    void     *si_ptr;     /* POSIX.1b signal */
    int      si_overrun;  /* Timer overrun count; POSIX.1b timers */
    int      si_timerid;  /* Timer ID; POSIX.1b timers */
    void     *si_addr;    /* Memory location which caused fault */
    long     si_band;     /* Band event (was int in glibc 2.3.2 and earlier) */
    int      si_fd;       /* File descriptor */
    short    si_addr_lsb; /* Least significant bit of address (since Linux 2.6.32) */
}
```
- si_value 是个union体，包含int sival_int 和 void \*sival_ptr; 两个字段，表示信号附带的数据,附带数据可以是一个整数也可以是一个指针
- 如果信号是 SIGCHLD，si_pid si_status si_uid 必须被设置
- 如果信号是 SIGBUS SIGILL SIGFPE SIGSEGV， si_addr 包含出错的内存地址(有可能该地址是无效的)
- si_errno 字段包含导致信号产生的错误的错误码
- si_code值 以及对应产生的原因详细可参考 man sigaction

### ucontext_t 结构体
```c
typedef struct ucontext {
  struct ucontext *uc_link;     /* pointer to context resumed when this context returns */
  sigset_t         uc_sigmask;  /* signals blocked when this context is active */
  stack_t          uc_stack;    /* stack used by this context */
  mcontext_t       uc_mcontext; /* machine-specific representation of saved context */
 ...
} ucontext_t;

```

### stack_t 结构体
```c
typedef struct {
  void  *ss_sp;     /* Base address of stack */
  int    ss_flags;  /* Flags */
  size_t ss_size;   /* Number of bytes in stack */
} stack_t;
```

## sigsetjmp 和 siglongjmp 函数
基本上和setjmp和longjmp相同，设置跳转点和跳转代码
```c
#include <setjmp.h>
int sigsetjmp(sigjmp_buf env, int savemask);
返回值：直接调用返回0，通过siglongjmp调用返回val值
void siglongjmp(sigjmp_buf env, int val);
```
- 如果savemask 值非0，sigsetjmp会将信号掩码保存到env中，siglongjmp调用时会恢复掩码
>备注：longjmp会自动将信号加入到信号掩码中，这可能导致信号掩码无法恢复

## sigsuspend函数
临时替换进程的信号掩码，并停止进程执行，直到收到已经注册调用信号处理函数的信号，或者终止进程
```c
#include <signal.h>
int sigsuspend(const sigset_t *sigmask);
```
- sigmask 不能阻塞SIGKILL 和 SIGSTOP信号
- 如果进程终止，sigsuspend将不会返回
- 如果捕获到信号，sigsuspend 在信号处理函数返回后返回，并且会重置信号掩码

## abort函数
中止进程
```c
#include <stdlib.h>
void abort(void);
```
abort发送了SIGABRT信号给调用进程，当执行此函数的时候，所有的stream都会被冲洗并且被关闭。

## System函数
system函数把command参数提交给命令解释器sh，调用的进程等待shell执行命令结束，忽略SIGINT、SIGQUIT，并且阻塞SIGCHLD信号
```c
#include <stdlib.h>
int system(const char *command);
返回值：如果command为空，shell可用返回非0值，shell不可用返回0；如果子进程不能被创建，或子进程的状态不可获取，返回-1；如果shell不能在子进程中执行，返回子进程调用_exit(127)的结果；如果成功，返回命令执行结果
```

## sleep、nanosleep和clock_nanosleep函数
暂停某个进程，直到seconds或request指定的时间后恢复，或在暂停阶段有捕获信号或中止进程
```c
#include <unistd.h>
unsigned int sleep(unsigned int seconds);
返回值：到期返回0，未到期时因信号中断则返回剩余时间
int nanosleep(const struct timespec *request, struct timespec *remain);
返回值：到期返回0，因捕获信号而中断调用或失败返回-1，如果remain不为空，恢复后记录剩余的时间，否则不记录
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain);
返回值：到期返回0，因捕获信号而中断调用或失败返回对应的ERRORS
```

### clock_nanosleep 函数

#### clock_id 参数
clock_id 用于指定时间类型，不同的时间类型影响最终暂停的效果
- CLOCK_REALTIME   这种类型的时钟可以反映wall clock time，用的是绝对时间，当系统的，时钟源被改变，或者系统管理员重置了系统时间之后，这种类型的时钟可以得到相应的调整，也就是说，系统时间影响这种类型的timer。
- CLOCK_MONOTONIC  用的是相对时间，他的时间是通过jiffies值来计算的。该时钟不受系统时钟源的影响，只受jiffies值的影响。
- CLOCK_PROCESS_CPUTIME_ID 本进程到当前代码系统CPU花费的时间

#### flags 参数
- flags为0 将request时间 被解释为相对时间
- flags为 TIMER_ABSTIME 将request时间解释为绝对时间

#### ERRORS
- EFAULT request or remain specified an invalid address
- EINTR  The sleep was interrupted by a signal handler
- EINVAL The value in the tv_nsec field was not in the range 0 to 999999999 or tv_sec was negative.
- EINVAL clock_id was invalid.  (CLOCK_THREAD_CPUTIME_ID is not a permitted value for clock_id.)

#### remain 参数
如果remain非空，并且flags不是TIMER_ABSTIME，remain将保存剩余时间；这个参数可以被用来再调用一次clock_nanosleep() 来完成需要的暂停

## sigqueue 函数
sigqueue通过队列向指定进程发送一个信号(signo参数)和数据(value参数)
```c
#include <signal.h>
int sigqueue(pid_t pid, int signo, const union sigval value)
返回值：成功返回0，出错返回-1
```

### sigqueue 的调用过程
1. 信号接收者使用int sigaction(int signum, const struct sigaction \*restrict act, struct sigaction \*restrict oldact)注册信号处理函数，并且其act参数的sa_flags设置为SA_SIGINFO；为act的 sa_sigaction 成员指定 信号处理函数
2. 信号发送者使用sigqueue发送信号

### 参数
- pid 指定进程，signo指定信号，value指定信号传递的参数
- signo = 0，不会发送信号，只检查pid的有效性以及当前进程是否有权限向目标进程发送信号
- 接收者捕获信号后，发送方调用sigqueue时传送的sigval value值会复制给接收方 sigaction act 参数的siginfo_t成员的si_value成员

### sigval union体
```c
typedef union sigval {
  int  sival_int;
  void *sival_ptr;
}sigval_t;
```

## 工作控制信号
POSIX.1 认为6个信号和作业控制相关
- SIGCHILD 子进程停止或终止
- SIGCONT 如果进程已经停止，使其恢复运行
- SIGSTOP 停止信号
- SIGTSTP 交互式停止信号
- SIGTTIN 后台进程组成员read控制中断
- SIGTTOU 后台进程组成员write控制中断

## 信号名和信号编号

### psignal psiginfo
psignal在stderr中打印给定信号的信号名
```c
#include <signal.h>
void psignal(int signo, const char *msg);
```
psiginfo在stderr中打印给定siginfo_t信号的内容
```c
#include <signal.h>
void psiginfo(const siginfo_t *info, const char *msg);
```
msg 参数作为打印信息的前缀

### strsignal sig2str str2sig
```c
#include <string.h>
char *strsignal(int signo);
返回值：返回字符串指针
```
strsignal打印给定信号的描述信息
```c
#include <signal.h>
int sig2str(int signo, char *str);
int str2sig(const char *str, int *signop);
返回值：成功返回0，出错返回-1
```
sig2str将signo转换为信号名，由str返回

sig2str将str给定信号名转换为信号值，由signop返回
