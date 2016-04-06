[TOC]
# 第8章 进程控制
内核是启动后获得控制权，内核有一部分就形成了pid为0的进程，也叫作调度进程，没有什么用，然后pid为1的进程被启动，也就是其他进程的父进程，一般都是init进程，它负责启动整个Unix系统，并将系统根据配置文件引导到一个可使用的状态，init和前面的调度进程不一样，调度进程实际上是内核的一部分，而init是内核启动的一个普通进程，但是它拥有root权限；还有很多和系统密切相关的内核进程，这些进程都以守护进程的形式常驻

## 进程标识
获取当前进程pid、父进程pid、真实用户ID、有效用户ID、真实组ID和有效组ID
```c
#include <unistd.h>
pid_t getpid(void);
pid_t getppid(void);

uid_t getuid(void);
uid_t geteuid(void);

gid_t getgid(void);
gid_t getegid(void);
返回值：出错返回-1

```
## 进程派生
- 新进程时对父进程的完整拷贝(包括描述符、环境变量,堆，栈等)
>备注：在实际的Unix系统实现中，常常使用差分存储的技术，也就是说，原来的堆栈不会被复制，两个进程以只读的形式共享同一个堆栈区域，当需要修改区域内容的时候，则在新的区域制作差分存储

- fork函数被调用一次，但是返回两次(分别作为父进程和子进程返回)，子进程和父进程都会得到返回值，但是子进程得到的返回值是0，父进程的返回值则是子进程的pid
```c
#include <unistd.h>
pid_t fork(void);
返回值: 子进程中返回0，父进程中返回子进程ID，出错返回-1
```
[fork1.c](../examples/proc/fork1.c)
```c
int		globvar = 6;		/* external variable in initialized data */
char	buf[] = "a write to stdout\n";

int
main(void)
{
	int		var;		      /* automatic variable on the stack */
	pid_t	pid;

	var = 88;
	if (write(STDOUT_FILENO, buf, sizeof(buf)-1) != sizeof(buf)-1)
		err_sys("write error");
	printf("before fork\n"); /* 这里故意不 flush stdout */

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid == 0) {   /* child */
		globvar++;             /* 子进程修改变量 */
		var++;
	} else {
		sleep(2);              /* 父进程休眠两秒 */
	}
	/* 子进程先打印，父进程休息两秒后打印 */
	printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar,
	  var);
	exit(0);
}
```

执行结果如下
```sh
$ ./fork1
a write to stdout
before fork
pid = 16105, glob = 7, var = 81
pid = 16104, glob = 6, var = 80
$ ./fork1 > temp
$ cat temp
a write to stdout
before fork
pid = 16105, glob = 7, var = 81
before fork
pid = 16104, glob = 6, var = 80
```
>备注：write函数是不带缓冲的IO,如果标准输出是连接到终端设备，那么它是行缓冲的，否则就是全缓冲的，在标准输出是终端设备的情况下，我们只看到了一行输出，因为换行符冲洗了缓冲区，而当标准输出重定向到文件的时候，输出是全缓冲的，这样换行符不会导致系统的自动写入，当fork函数执行的时候，这行输出依旧被存储在缓冲区中，然后随着fork函数被共享给了子进程，随着后续继续的写入，两个进程都同时写入了before fork字符串。

### 文件共享
所以对于派生子进程，有以下两种方式处理文件描述符
1. 父进程使用函数等待子进程完成。这个非常简单，由于共享同一个文件表项，子进程的输出也会更新父进程的偏移量，所以等待子进程完成后直接就能读写。
2. 父进程和子进程各自执行不同的程序段。在这种情况下，在fork以后，父子进程各自只使用不冲突的文件描述符。

### 使用fork函数有两种原因
1. 父进程复制自身，各自执行不同的代码段，也就是网络服务中典型的多进程模型。
2. 一个进程想要执行不同的程序。shell就是这样的，所以子进程可以在fork后立刻使用exec，让新程序运行。

### vfork 函数
vfork函数创建一个新进程，但是不完全拷贝父进程地址空间
- 加快函数执行
- 保证子进程先运行
```
#include <unistd.h>
pid_t vfork(void);
```
>备注：vfork函数只是用于spawn一个进程的前置操作，而不是正常的派生子进程


[vfork1.c](../examples/proc/vfork1.c)
```c
#include "apue.h"

int		globvar = 6;		/* external variable in initialized data */

int
main(void)
{
	int		var;		/* automatic variable on the stack */
	pid_t	pid;

	var = 88;
	printf("before vfork\n");	/* we don't flush stdio */
	if ((pid = vfork()) < 0) {
		err_sys("vfork error");
	} else if (pid == 0) {		/* child */
		globvar++;				/* 子进程修改父进程的变量 */
		var++;
		_exit(0);				/* 子进程结束 */
	}

	/* 父进程执行，发现变量已经改变 */
	printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar,
	  var);
	exit(0);
}

```
>备注：子进程使用_exit 函数关闭，因为子进程和父进程共享同一个内存空间，而exit会在关闭进程钱进行一系列清理操作，可能会导致父进程的数据被清理

## 退出进程
无论进程是正常退出还是异常退出，实际上在最后都需要内核进行执行清理工作，对于正常退出，都会有一个退出状态可以传递，对于异常终止，内核同样会产生一个终止状态，最终，都会变成退出状态，这样父进程就能得到子进程的退出状态。

父进程可能会先于子进程结束，但是实际上在终止每个进程的时候，内核会检查所有现有的进程，如果是终止子进程时父进程已经结束，就将其父进程修改为init进程，也就是pid为1的进程。

## wait 函数族
当一个进程退出，无论是正常还是异常，内核都会向父进程发送SIGCHLD信号，在默认情况下，都是选择忽略这个信号。

wait函数会阻塞父进程，然后自动分析是否有当前进程的某个子进程已经终止，如果是，收集这个子进程的信息，并把它彻底销毁后返回，如果没有，wait就会一致阻塞在这里，直到有一个出现

### wait，waitpid
wait和waitpid函数就在于参数的区别，waitpid可以传入一个options选项用于行为的改变，还有就是可以等待指定ID的进程退出，wait则是等待第一个子进程的退出；stat_loc将会包含进程结束信息。
```c
#include <sys/wait.h>
pid_t wait(int *stat_loc);
返回值: 成功返回子进程ID，出错返回-1
pid_t waitpid(pid_t pid, int *stat_loc, int options);
返回值: 成功返回子进程ID，或者0，出错返回-1
```

#### 检查进程结束信息
POSIX.1 定义了四个宏用来判断进程结束状态
| 宏     | 说明     |
|:--|:--|
| WIFEXITED(status)       | 如果status在子进程正常结束时返回，返回true。这时可以执行 WEXITSTATUS(status) 获取 exit _exit 或 _Exit 传递参数的低八位      |
| WIFSIGNALED(status)       | 如果status在子进程异常终止(接收到一个不可捕捉的信号)时返回，返回true。这时可以执行 WTERMSIG(status) 获取 子进程终止的信号number。另外，有些实现定义宏WCOREDUMP(status)，如果产生终止进程的 core 文件，则返回真      |
| WIFSTOPPED(status)       | 如果status在子进程暂停时返回，返回true。这时可以执行 WSTOPSIG(status) 获取导致子进程暂停的信号number。    |
| WIFCONTINUED(status)       | 如果status在子进程(暂停后)恢复继续时返回，返回true。   |

#### options 参数
| options 参数     | Header Two     |
|:--|:--|
| WCONTINUED       | 当OS支持作业控制，如果pid指定的进程暂停后后继续，但是其状态未报告，返回其状态      |
| WNOHANG       | 如果 pid 指定的进程的退出状态当前不可用，则 waitpid不阻塞该进程，并且返回值为0       |
| WUNTRACED       | 当OS支持作业控制，如果pid指定的进程暂停，但是其状态未报告，返回其状态；WIFSTOPPED宏以确定返回值是否对应这个暂停的子进程       |

### waitid
对waitpid进行扩展
```c
#include <sys/wait.h>
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
返回值: 成功返回0，出错返回-1
```

#### idtype 参数
| idtype 参数     | 描述     |
|:--|:--|
| P_PID       | 指定特定的进程      |
| P_PGID       | 指定的进程组      |
| P_ALL       | 任意进程，忽略 id 参数       |

#### options 参数
| options 参数     | 描述     |
|:--|:--|
| WCONTINUED       | 如果pid指定的进程暂停后后继续，但是其状态未报告，返回其状态      |
| WEXITED       | 返回那些被退出的进程的状态      |
| WNOHANG       | 如果 pid 指定的进程的退出状态当前不可用，则 waitid不阻塞该进程，并且返回值为0       |
| WNOWAIT       | 忽略子进程的退出状态，交个后续的 wait waitid waitpid 处理    |
| WSTOPPED       | 如果pid指定的进程暂停，但是其状态未报告，返回其状态；       |

#### infop 参数
info 参数指向 siginfo 结构体的指针，该结构体包含了因为子进程状态改变而生成的信号的详细信息

### wait3 和 wait4
继承自BSD
```c
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

pid_t wait3(int *stat_loc, int options, struct rusage *rusage);
pid_t wait4(pid_t pid, int *stat_loc, int options, struct rusage *rusage);
返回值：成功返回进程ID，出错返回-1
```

#### rusage 结构体
rusage 结构体包含用户cpu时间总量，系统cpu时间重量，页面出错次次数，接收到信号的次数等信息
```c
struct	rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	long	ru_maxrss;	/* maximum resident set size */
	long	ru_ixrss;	/* integral shared memory size */
	long	ru_idrss;	/* integral unshared data size */
	long	ru_isrss;	/* integral unshared stack size */
	long	ru_minflt;	/* page reclaims */
	long	ru_majflt;	/* page faults */
	long	ru_nswap;	/* swaps */
	long	ru_inblock;	/* block input operations */
	long	ru_oublock;	/* block output operations */
	long	ru_msgsnd;	/* messages sent */
	long	ru_msgrcv;	/* messages received */
	long	ru_nsignals;	/* signals received */
	long	ru_nvcsw;	/* voluntary context switches */
	long	ru_nivcsw;	/* involuntary " */
};
```

## 竞争条件
当子进程和父进程竞争资源时，在父进程中调用wait 和 wait3 函数可以让子进程先执行(waitpid waitid wait4 也是一样，区别就是在父进程中指定子进程ID)。但是，如果需要保证父进程执行，就需要使用Unix系统的信号机制。不过 wait 函数本质上也是使用信号机制
[tellwait1.c](../examples/proc/tellwait1.c)
```c
#include "apue.h"
#include <sys/wait.h>

int
main(void)
{
	pid_t	pid;
	int		status;

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid == 0) {
		....
		exit(0);       /* 先执行子进程 */
	} else {
		wait(&status); /* 阻塞在这里，等待子进程退出后再执行后续内容 */
    ....
	}
	exit(0);
}
```

## exec函数族
exec 用于调用一个可执行文件，调用时进程ID不变，仅仅将当前环境替换为可执行文件的正文、数据、堆、栈。一般在fork创建的子进程中使用
```c
#include <unistd.h>
int execl(const char *path, const char *arg0, ... /*, (char *)0 */);
int execv(const char *path, char *const argv[]);
int execle(const char *path, const char *arg0, ... /*, (char *)0, char *const envp[] */);
int execve(const char *path, char *const argv[], char *const envp[]);

int execlp(const char *file, const char *arg0, ... /*, (char *)0 */);
int execvp(const char *file, char *const argv[]);
int fexecve(int fd, char *const argv[], char *const envp[]);
返回值：成功无返回值，出错返回-1
```
- path 或 file 参数： 如果 参数中 包含 / 时，则视为文件的绝对路径，否则就使用PATH环境变量中的可执行文件
- 如果 path 或 file 指定的文件时 shell 脚本，就尝试使用 /bin/sh 执行文件
- execl* 和 execv*的区别在于 execl* 的参数列表为字符串(最后一个参数必须是(char \*)0)，而execv 的参数列表为各个字符串的地址的封装进指针数组中
- exec*e 可以传递包含环境变量字符串地址的指针数组，如果没有次参数，则复制现有的环境变量；其他函数则使用进程中的environ变量复制先用的环境变量
- 参数列表和环境变量的长度是受限的，由ARG_MAX确定，至少4096个字节
- 这些函数最终都是execve系统调用来完成实际工作
- exec前需要进行权限检查

## 修改进程的uid gid
进程拥有三种ID：实际用户(组)ID(real user ID)用于标识进程是谁拥有负责的，有效用户(组)ID(effective user ID)则是用于权限检查的，保存的设置用户(组)ID(Saved set-user ID)则是用于恢复有效用户ID(其值是最近一次setuid系统调用或是exec一个setuid程序的结果)

### setuid setgid
setuid setgid 函数为当前进程设置实际用户(组)ID、有效用户(组)ID和保存的设置用户(组)ID为指定值
```c
#include <unistd.h>
int setuid(uid_t uid);                 /*设置实际uid*/
int setgid(gid_t gid);                 /*设置实际gid*/
返回值：成功返回0，出错返回-1
```
- root权限进程才能更改实际用户(组)ID
- 只有程序文件设置了set-uid 位，exec函数族才会设置有效用户(组)ID，有效用户(组)ID只能是实际用户(组)ID或者保存的设置用户(组)ID
- 设置用户(组)ID是exec函数复制原先的有效用户(组)ID得到的，如果程序设置了set-uid位，那么就会存在这个值

### seteuid setegid
只修改有效用户(组)ID
```c
#include <unistd.h>
int seteuid(uid_t euid);               /*只设置有效uid*/
int setegid(gid_t egid);               /*只设置有效gid*/
返回值：成功返回0，出错返回-1
```
- root权限进程可以任意修改有效用户组ID
- 非特权用户只能将其设置为实际用户组ID或者保存设置用户组ID

### setreuid setregid
root权限可以分别设置实际用户(组)ID和有效用户(组)ID，非特权用户只能交换实际用户(组)ID和有效用户(组)ID，如果其中的任意参数为-1，则相应的ID保持不变
```c
#include <unistd.h>
int setreuid(uid_t ruid, uid_t euid);  /*分别设置有效uid和实际uid*/
int setregid(gid_t rgid, gid_t egid);  /*分别设置有效gid和实际gid*/
返回值：成功返回0，出错返回-1
```

## system 函数
system函数将字符串传递给sh解释器，并且一直会等待shell结束执行，如果command参数是NULL指针，并且sh解释器存在，则返回非0，否则就是0，这主要用于判断sh存在与否
>备注: system 函数实际上调用了fork exec 和waitpid
```c
#include <stdlib.h>
int system(const char *cmdstring);
返回值：如果fork exec waitpid出错返回-1，如果exec失败(不能执行shell)，返回值则是shell执行exit(127),如果成功，返回shell的结束状态
```

## 用户标识
用户标识是uid gid 对应的英文标识
```c
#include <unistd.h>
char *getlogin(void);           /*修改登录名*/
int setlogin(const char *name); /*设置登录名*/
```

## 进程调度
进程调度是由操作系统决定，进程优先级可以自行决定

### nice 函数
inc数值越大则优先级越低，超级用户可以使用负的inc 值，使优先顺序靠前，进程执行较快
```c
#include <unistd.h>
int nice(int inc);
返回值：成功返回新nice值，出错返回-1
```

### getpriority setpriority
而who参数的解释取决于which参数，当who为0时，表示当前的进程、进程组或用户，而prio则是-20到20的值，默认为0，低优先级则有高CPU使用。
```c
#include <sys/resource.h>
int getpriority(int which, id_t who);
返回值：成功返回nice值(−NZERO 到 NZERO−1 之间)，出错返回-1
int setpriority(int which, id_t who, int value);
返回值：成功返回0，出错返回-1
```

## 进程时间
tms 结构体中记录用户CPU时间、系统CPU时间、包含子进程的用户CPU时间和包含子进程的系统CPU时间，times返回实际时间
```c
#include<sys/times>
clock_t times(struct tms *buffer);
```

### tms 结构体
```c
struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
    clock_t tms_cutime;
    clock_t tms_cstime;
};
```
