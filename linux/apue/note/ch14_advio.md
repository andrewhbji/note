[TOC]
# 第14章 高级I/O

## 非阻塞I/O
系统调用分高速、低速两种，低速系统调用会导致系统永久性阻塞，包括：
- 管道、网络、终端设备中的数据不存在
- 管道、网络无法接收数据
- 访问文件或数据时条件未成立
- 读写已经加上强制性记录锁的文件
- 使用某些 ioctl 操作
- 某些进程间通信函数

对于 open、read、write 等 I/O 函数，如果出现操作不能完成就立即出错返回，并不会导致系统阻塞，这样的I/O就是非阻塞I/O

在使用 open 或 fcntl 函数的时候指定 O_NONBLOCK 标志，就会使用非阻塞I/O
>备注：非阻塞I/O可能会丢失数据，在大量传输数据的时候容易出现系统调用大量失败的情况

## 记录锁
在多方一起操作文件的某个部分的时候，为保证文件的读写正确， UNIX 系统提供了记录锁机制

### flock 结构体
```
struct flock {
    off_t l_start;    /* starting offset */
    off_t l_len;      /* len = 0 means until end of file */
    pid_t l_pid;      /* lock owner */
    short l_type;     /* lock type: read/write, etc. */
    short l_whence;   /* type of l_start */
};
```
- l_type 指定记录锁类型，可指定为 F_RDLCK（共享性读锁）、F_WRLCK（独占性写锁）、F_UNLCK（解锁）
- l_start 和 l_whence 指定加锁或解锁区域的位置和偏移量， l_whence 可指定为 SEEK_SET、SEEK_CUR、SEEK_END
- l_pid 指定进程持有的锁能阻塞当前进程
- l_len 指定加锁或解锁区域的长度，如果 l_len 为 0，则表示所得范围可以扩展到最大可能偏移量，这意味着不管向文件追加了多少数据，都在锁的范围之内；为了对整个文件加锁，可以设置 l_start 和 l_whence 指向文件的起始位置，l_len 为 0

### 记录锁的兼容性
记录锁的读写兼容关系如下：
| 当前状态  | 建立读锁  | 建立写锁  |
|:--|:--|:--|
| 无锁  | 允许  | 允许  |
| 有读锁  | 允许  | 拒绝  |
| 有一个写锁  | 拒绝  | 拒绝  |

### fcntl 函数
fcntl 可以用来设置记录锁，使用 flock 结构体描述 这个锁， cmd 用来指定以什么方式设置
```
int fcntl(int fd, int cmd, ... /* struct flock *flockptr */);
返回值：若成功返回值依赖 cmd 参数，失败返回-1
```
- cmd 为 F_GETLK 时， fcntl 会测试是否能建立记录锁。fcntl 会判断 fd 文件是否已有和 flockptr 相排斥的记录锁，如果有，将 flockptr 指向已有的 flock ；如果没有，将 flockptr 指向的 flock 的 l_type 设置为 F_UNLCK
- cmd 为 F_SETLK 时， fcntl 会企图建立 flockptr 所描述的记录锁。这时系统会根据记录锁的兼容性关系决定是否创建锁，如果系统阻止创建，fcntl会立即出错返回，此时 errno 被设置为 EACCES 或 EAGAIN
- cmd 为 F_SETLKW 时， 和 F_SETLK 基本一致，只不过 fcntl 调用会被阻塞，直到先前的锁已经被解除

### fcntl 记录锁使用
一般先用 F_GETLK 测试是否能建立锁，然后使用 F_SETLK 或 F_SETLKW 建立锁。 这两个操作不是原子操作，所以多进程时容易导致冲突

### 记录锁的实现方式
在设置释放锁的时候，内核是根据字节数维持锁的范围的。实际上内核通过一个flock结构体的链表来维护所有记录锁，每次的变更锁都需要遍历链表，来合并或拆分flock节点。

### 记录锁的继承和释放
记录锁的继承和释放有三条规则：
- 锁和进程、文件相关联，换言之，一个进程结束的时候，所有的锁全部释放，这实际上是exit函数做清理的，第二就是文件描述符关闭的时候，该文件所有的锁都会关闭
- fork产生的子进程不继承父进程的锁。因为锁是用于限制多个进程读写同一个文件的，如果fork能继承锁，那就起不到约束作用
- 执行exec后，新程序继承原执行程序的锁。如果文件open时设置了close_on_exec，执行exec会自动释放锁

## I/O多路转接
对于内核，I/0只分阻塞和非阻塞

对于阻塞的I/O，从一个描述符读，然后写到另一个描述符，一般使用循环
```c
while ((n = read(STDIN_FILENO, buf, BUFSIZE)) > 0)
    if (write(STDOUT_FILENO, buf, n) != n)
  err_sys("write error");
```

如果需要从两个描述符读(如telnet客户端程序)，由下面几种方法：
- 使用两个进程(或线程)分别处理一路读写，在信号处理(或处理线程同步)上会很复杂
- 使用异步I/O，当一个描述符就绪，内核会发出信号，需要进程处理信号。由于这种信号对每个进程而言只有一个(SIGPOLL或SIGIO)，这对于潜在的要读取的文件数量来说是不足的
- 使用非阻塞I/O读，其基本思想是轮询每个描述符，如果有数据就处理，没有数据就立即返回，每次循环之间间隔一段时间，这样做导致浪费cpu时间

- 使用I/O多路转接，本质上这是一种异步阻塞I/O，首先构造一个列表维护潜在描述符，然后调用一个函数，函数被调用后会阻塞，直到这些描述符中的一个准备好I/O时，这个函数才返回。poll、pselect 和 select 这三个函数用于I/O多路转接，这些函数返回时，进程会被告知哪些描述符已经准备好可以进行I/O

### select 和 pselect 函数
select 和 pselect 调用时会被阻塞，直到 fd_set 指定的描述符可用，或者等待超时后返回
```c
#include <sys/select.h>
int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout);
int pselect(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, const struct timespec *restrict timeout, const sigset_t *restrict sigmask);
返回值：成功返回就绪的描述符数目；超时返回0；出错返回-1
```
- fd_set 用于存储描述符， readfds 、writefds 、errorfds 分别关注用于可读、可写、以及处理错误的描述符；下面的函数用于处理 fd_set 
```c
#include <sys/select.h>
void FD_CLR(fd, fd_set *fdset); /* 从 fdset 中清理 fd */
void FD_COPY(fd_set *fdset_orig, fd_set *fdset_copy); /* 复制 fdset_orig */
int FD_ISSET(fd, fd_set *fdset); /* 判断 fdset 关联的 fd 是否可读写  */
void FD_SET(fd, fd_set *fdset); /* 将 fd 加入 fdset */
void FD_ZERO(fd_set *fdset); /* 清空 fdset */
```
- timeout 用于指定超时时间；timeout == NULL，永远等待；timeout->tv_sec == 0 && timeout->tv_usec == 0，不等待；timeout->tv_sec != 0 || timeout->tv_usec != 0，等待指定时间
- nfds 是 fdset 中的最大fd编号值 + 1

可以在循环中使用 select 和 pselect 进行I/O多路转接操作， 函数返回时，如果返回值大于0，则表示 fdset 中有至少一个fd就绪，然后使用 FD_ISSET 函数检查哪个fd可读写，进而使用该fd进行相应处理

pselect 以原子操作的形式为select函数安装信号掩码，并在select返回后恢复之前的信号掩码，这样，在pselect等待描述符就绪的时候，不会被指定的信号打断
>备注：pselect 等价于执行
```c
sigset_t origmask;
sigprocmask(SIG_SETMASK, &sigmask, &origmask);
select(nfds, &readfds, &writefds, &exceptfds, timeout);
sigprocmask(SIG_SETMASK, &origmask, NULL);
```

### poll 函数
poll 和 selcet 功能差不多，但 poll 效率更高

poll 函数通过 pollfd 结构体判断其中的fd和预期事件是否发生来判断fd是否可用，如果预期事件发生或者等待超时，poll将返回
```c
#include <sys/select.h>
int poll(struct pollfd fds[], nfds_t nfds, int timeout);
返回值：成功返回就绪的描述符数目；超时返回0；出错返回-1
```

pollfd 结构体中包含 fd、预期条件 以及 发生事件，如果 poll 返回时，events 指定的预期事件没有发生， revents 为空。可以通过判断 revents 和 events 是否一致来判断 fd 是否就绪
```c
struct pollfd {
    int    fd; /* 描述符 */
    short  events;   /* 预期事件 */
    short  revents;  /* 实际发生事件 */
};
```
- 可用的 envents 值包括
| envents值  | 说明  |
|:--|:--|
| POLLIN  | 普通或优先级带数据可读  |
| POLLRDNORM  | 普通数据可读  |
| POLLRDBAND  | 优先级带数据可读  |
| POLLPRI  | 高优先级数据可读  |
| POLLOUT  | 普通数据可写  |
| POLLWRNORM  | 普通数据可写  |
| POLLWRBAND  | 优先级带数据可写  |
| POLLERR  | 发生错误  |
| POLLHUP  | 发生挂起  |
| POLLNVAL  | 描述字不是一个打开的文件  |

## 异步I/O

### BSD异步I/O
对于BSD系列的系统来说，异步IO信号是SIGIO和SIGURG信号的组合，SIGIO是通用异步IO的信号，SIGURG则是通知网络连接的数据已经到达。

### POSIX异步I/O
POSIX标准为不同类型文件的异步I/O操作提供了一致的方式

#### AIO 控制块
AIO控制块 aiocb 结构体用来描述I/O操作
```c
#include <aiocb.h>
struct aiocb {
    int       aio_fildes;     /* 指定被打开的文件描述符 */
    off_t     aio_offset;     /* 指定操作的偏移量 */
    volatile void   *aio_buf; /* 指定用于I/O操作的缓冲区 */
    size_t    aio_nbytes;     /* 指定读写的字节数 */
    int       aio_reqprio;    /* 指定I/O操作的优先级 */
    struct    sigevent aio_sigevent;   /* 用于指定I/O完成后的通知方式 */
    int       aio_lio_opcode; /* 指定是使用 aio_read 函数还是 aio_write 函数执行异步I/O，只能用于使用lio_listio函数发送异步I/O请求 */
};
```

sigevent 用来描述通知方式
```c
struct sigevent {
    int    sigev_notify; /* 指定通知的类型 */
    int    sigev_signo;  /* 指定通知产生的信号数 */
    union sigval sigev_value;  /* 传递给 sigev_notify_function 的参数 */
    void (*sigev_notify_function) (union sigval); /* I/O完成后在线程中调用的函数 */
    void  *sigev_notify_attributes; /* 指定 sigev_notify_function 函数的调用线程的属性 */
    pid_t  sigev_notify_thread_id; /* ID of thread to signal (SIGEV_THREAD_ID) */
};
union sigval {    /* Data passed with notification */
    int    sival_int;   /* Integer value */
    void   *sival_ptr;  /* Pointer value */
 };
```

通知的方式由四种，由sigev_notify指定
- SIGEV_NONE    异步I/O完成后，不通知进程
- SIGEV_SIGNAL  异步I/O完成后，产生 sigev_signo 字段指定的信号
- SIGEV_THREAD  异步I/O完成后，调用 sigev_notify_function 函数，函数的参数是 sigev_value，这个函数默认在一个分离态的线程中执行；可以通过为 sigev_notify_attributes 指定一个 pthread_attr_t 结构体来设置sigev_notify_function 的调用线程的属性
- SIGEV_THREAD_ID Linux 特有的，目前仅仅创建POSIX计时器使用(参考 timer_create(2))

#### 执行异步I/O
aio_read aio_write 函数用于发出异步I/O请求， aio_fsync 函数用于强制处理所有异步I/O请求
```c
#include <aio.h>
int aio_read(struct aiocb *aiocbp);
int aio_write(struct aiocb *aiocbp);
int aio_fsync(int op, struct aiocb *aiocb);
返回值:成功返回0，出错返回-1
```
调用 aio_read aio_write 函数后，异步I/O请求就被放在等待处理队列中。函数的返回值和I/O处理结果无关，如果想要强制处理所有请求，需要调用aio_fsync函数

aio_fsync 函数的 op 参数指定将缓冲区写入磁盘的方式
- O_DSYNC 会调用 fdatasync 函数
- O_SYNC  会使用 fsync 函数

#### 异步I/O的操作结果
aio_error 函数用于获取 aio_read aio_write aio_fsync 的完成状态
```c
#include <aio.h>
int aio_error(const struct aiocb *aiocbp);
返回值：aio完成返回0；aio_error 调用失败，返回-1，通过errno会告诉原因；如果异步I/O请求仍然处于等待中，返回EINPROGRESS；其他情况返回对应错误码
```

aio_return 函数用于返回I/O执行的结果
```c
#include <aio.h>
ssize_t aio_return(struct aiocb *aiocbp);
```
标准I/O的返回状态直接通过 read 和 write 函数返回获得，但是异步I/O不能立即得到返回状态，因为 aio_read 和 aio_write 在调用由没有阻塞；而且只有使用 aio_error 确认异步I/O完成后，才能使用 aio_return 返回异步I/O的执行结果

#### aio_suspend 和 aio_cancel
如果异步I/O长时间未完成，可以使用 aio_suspend 将线程阻塞，等待异步I/O完成，或者可以使用 aio_cancel 取消异步I/O
```c
#include <aio.h>
int aio_suspend(const struct aiocb *const list[], int nent, const struct timespec *timeout);
返回值: 成功返回0，出错返回-1
int aio_cancel(int fd, struct aiocb *aiocbp);
返回值: 所有操作都完成返回 AIO_ALLDONE;所有操作都取消返回 AIO_CANCELED；至少有一个要求的操作没有取消返回AIO_NOTCANCELED；aio_cancel 调用失败返回-1，通过errno获取错误码
```
- fd 用于指定正在进行异步I/O操作的文件描述符，如果 aio_cancel 的 aiocbp 参数没有指定，则取消掉 fd 上的所有异步I/O操作

#### lio_listio 函数
lio_listio 可以同时发起多个I/O请求，可以以异步的方式使用，也可以以同步的方式使用。一般用于在一个系统调用中启动大量I/O操作
```c
int lio_listio(int mode, struct aiocb *restrict const list[restrict],int nent, struct sigevent *restrict sigev);
返回值:成功返回0，失败返回-1
```
- mode 参数决定是否是异步I/O。设置为 LIO_WAIT (同步)，则 lio_listio 函数将在所有I/O操作完成后返回；设置为 LIO_NOWAIT (异步), lio_listio 调用后立即返回
- list 参数用于保存多个 aiocb 控制块
- nent 参数用于指定list中aiocb控制块的数量
- sigev 参数指定所有异步I/O完成后的通知，可以设置为空；当 mode 设置为 LIO_WAIT 时， sigev 设置被忽略

当时用 lio_listio 函数发送异步I/O请求时， aiocb 控制块结构体的 aio_lio_opcode 需要被指定为LIO_READ、LIO_WRITE、lIO_NOP，以告诉 lio_listio 是调用 aio_read 还是 调用 aio_write 还是将被忽略的空操作

UNIX环境的实现会限制lio_listio可以操作异步I/O的数量：
- AIO_LISTIO_MAX 常量用于限制单个I/O列表的最大I/O操作数(使用 sysconf(_SC_IO_LISTIO_MAX) 查看)
- AIO_MAX 限制等待中的异步I/O操作的最大数(使用 sysconf(_SC_AIO_MAX) 查看)
- AIO_PRIO_DELTA_MAX 限制进程可以减少的其异步I/O优先级的最大值(使用 sysconf(_SC_AIO_PRIO_DELTA_MAX) 查看)

## readv 和 writev 函数
readv 和 writev 函数用于在一次函数调用中读写多个非连续缓冲区
```c
#include <sys/uio.h>
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
返回值:成功返回已读或已写的字节数；出错返回-1
```
- iov 是个指向iovec结构体数组的指针
- iovec 结构体用来每个缓冲区的起始位置和长度
```c
struct iovec{
    void *iov_base; /* 缓冲区起始位置 */
    size_t iov_len; /* 缓冲区大小 */
}
```
- iovcnt 指定需要读写的缓冲区的数量

## 存储映射I/O
存储映射I/O机制能将一个磁盘文件映射到内存空间上，于是，当读写内存中数据的时候，自动读写磁盘文件，不需要调用 read 和write

### mmap 函数
mmap 函数通过内核实现存储映射I/O功能
```c
#include <sys/mman.h>
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
返回值：成功返回映射内存的起始地址；出错返回MAP_FAILED
```
- fd 指定文件描述符。使用 mmap 函数前，必须打开该文件
- addr 指定映射内存的起始地址，通常设置为0，表示由系统分配该地址
- len 指定映射内存的字节数
- offset 指定映射到内存的内容在文件中的起始偏移量
- prot 指定映射内存的保护要求，prot 可以指定为 PROT_NONE ,也可以指定为 PROT_READ PROT_WRITE PROT_EXEC 任意组合的按位或，但是必须是基于文件描述符的打开方式
| prot 值  | 说明  |
|:--|:--|
| PROT_READ  | 内存可读  |
| PROT_WRITE  | 内存可写  |
| PROT_EXEC  | 内存可执行  |
| PROT_NONE  | 内存不可访问  |

- flags 指定被映射内容的类型，映射选项和映射内存是否可以共享。它的值可以是一个或者多个以下位的组合体
| flags 值  | 说明  |
|:--|:--|
| MAP_FIXED  | 强制使用addr为内存固定地址，如果指定的内存区和现有内存区重复，重叠区域将被废弃；如果addr不可用，操作失败一般不推荐使用  |
| MAP_SHARED  | 所有映射fd文件的进程共享这个映射内存，共享内存的变更不会立即写回到文件中，内核会在某个时刻根据虚拟内存算法自动进行  |
| MAP_PRIVATE | 创建映射内存时会创建被映射文件的副本，对映射内存读写只影响这个副本  |
| MAP_NORESERVE  | 不为内存保留交换空间，如果内存不足，写内存时会产生SIGSEGV(内存不可访问)信号  |
| MAP_LOCKED  | 锁定内存，防止被转移到交换空间  |
| MAP_GROWSDOWN  | 用于堆栈，告诉内核虚拟内存系统，内存可以向下扩展  |
| MAP_ANONYMOUS  | 匿名映射，内存不与任何文件关联  |
| MAP_32BIT  | 将内存放在进程地址空间的低2GB位，MAP_FIXED指定时会被忽略。当前这个标志只在x86-64平台上得到支持  |
| MAP_POPULATE  | 为文件映射通过预读的方式准备好分页表。随后对内存的访问不会被页违规阻塞  |
| MAP_NONBLOCK  | 仅和MAP_POPULATE一起使用时才有意义。不执行预读，只为已存在于内存中的分页建立分页表入口  |

offset 和 addr 通常被要求是系统虚拟内存分页长度的倍数，如果文件长度小于分页长度，则offset只能为0。虚拟内存分页长度可用带参数的_SC_PAGESIZE 或 _SC_PAGE_SIZE 的 sysconf 函数查询

SIGSEGV 和 SIGBUS 信号与映射内存相关，如果内存只读，尝试写内存时会产生 SIGSEGV 信号；如果访问映射内存之前另一个进程已将文件截断，此时访问被截断部分对应的映射内存会产生 SIGBUS 信号

子进程能通过 fork 继承映射内存，调用exec后映射内存将丢弃

### mprotect 函数
mprotect 函数用于修改现有映射内存及其权限
```c
#include <sys/mman.h>
int mprotect(void *addr, size_t len, int prot);
返回值：成功返回0；出错返回-1
```
- 参数和mmap函数一致
- 对于共享内存，如果只变更一个字节，整个分页都被写回

### msync 函数
msync 函数用于将共享内存中分页的变更回写到文件
```c
#include <sys/mman.h>
int msync(void *addr, size_t len, int flags);
返回值：成功返回0；出错返回-1
```
- flags 为 MS_ASYNC 时为异步调用，为MS_SYNC 时函数在回写完毕后返回，MS_SYNC 和 MS_ASYNC 必须选一个；MS_INVALIDATE 是课选的，用于通知操作系统丢弃掉哪些和底层存储器没有同步的分页

### munmap 函数
munmap 函数用于解除文件映射，进程终止时会自动解除文件映射。解除文件映射不会触发将内存回写到硬盘文件
```c
#include <sys/mman.h>
int munmap(void *addr, size_t len);
返回值：成功返回0；出错返回-1
```