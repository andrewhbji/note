[TOC]
# 第12章 线程控制

## 线程相关限制
sysconf 函数可以查询线程相关的限制，包括
| 名称 | 描述 | name 参数 | 值  |
|:--|:--|:--|:--|
| PTHREAD_DESTRUCTOR_ITERATIONS  | 线程退出时，操作系统最多可以执行几轮清理线程指定数据  | _SC_THREAD_DESTRUCTOR_ITERATIONS  | 4 |
| PTHREAD_KEYS_MAX  | 线程可以创建的键(用于线程指定数据)的最大数  | _SC_THREAD_KEYS_MAX  | 1024 |
| PTHREAD_STACK_MIN  | 线程可用栈的最小字节数  | _SC_THREAD_STACK_MIN  | 16384  |
| PTHREAD_THREADS_MAX  | 进程可以创建线程的最大数  | _SC_THREAD_THREADS_MAX  |  没有限制  |

## 线程属性
pthread_attr_t 用来在调用 pthread_create 时 设置线程的四个属性: detachstate guardsize stackaddr stacksize

### pthread_attr_init pthread_attr_destroy
pthread_attr_init 初始化 pthread_attr_t，pthread_attr_destroy 销毁 pthread_attr_t
```c
#include <pthread.h>
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
返回值：成功返回0，失败返回错误编号
```

### detachstate 属性
pthread_attr_getdetachstate 和 pthread_attr_setdetachstate 分别用来设置和获取 detachstate 属性
```c
#include <pthread.h>
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
返回值：成功返回0，失败返回错误编号
```
detachstate 可以设置为两个值
- PTHREAD_CREATE_DETACHED: 以分离状态启动线程，等价于使用 pthread_detach 函数
- PTHREAD_CREATE_JOINABLE: 正常启动线程，应用程序可以获取线程的终止状态

### stackaddr stacksize 属性
进程的虚地址空间大小是固定的，线程共享这些虚地址空间。如果应用使用了太多线程，当线程栈的累计大小超过了可用空间时会导致溢出，就需要由程序接管线程栈的管理，使用 malloc 或者 mmap 为线程分配可替代的的栈空间

stackaddr 指定新线程栈的初始地址，stacksize 设置新线程栈的大小

pthread_attr_getstack 和 pthread_attr_setstack 函数用于设置和获取线程栈属性
```c
#include <pthread.h>
int pthread_attr_getstack(const pthread_attr_t *restrict attr, void **restrict stackaddr,size_t *restrict stacksize);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
返回值：成功返回0，失败返回错误编号
```

也可以使用 pthread_attr_getstacksize 和 pthread_attr_setstacksize 专门设置和获取 stacksize 属性
```c
#include <pthread.h>
int pthread_attr_getstacksize(const pthread_attr_t *restrict attr, size_t *restrict stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
返回值：成功返回0，失败返回错误编号
```
>备注：线程栈属性在 SUS 中的 XSI 选项中定义，现行 UNIX 环境都支持。如果需要检查系统是否支持线程栈属性，可以在编译阶段使用 _POSIX_THREAD_ATTR_STACKADDR 和 _POSIX_THREAD_ATTR_STACKSIZE 符号，也可以在运行阶段使用 sysconf 检查 _SC_THREAD_ATTR_STACKADDR 和 _SC_THREAD_ATTR_STACKSIZE 字段

### guardsize 属性
UNIX提供线程栈缓冲机制，以线程栈溢出，guardsize 属性控制线程栈扩展内存的大小，一般设置为非0值以避免线程栈溢出。这个属性的默认值根据Unix的实现而定

如果修改了 stackaddr 属性，则 guardsize 属性失效，等同于将 guardsize 属性设置为0，这样系统就认为由程序接管线程栈的管理

pthread_attr_getguardsize 和 pthread_attr_setguardsize 用于设置和获取 guardsize 属性
```c
#include <pthread.h>
int pthread_attr_getguardsize(const pthread_attr_t *restrict attr, size_t *restrict guardsize);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
返回值：成功返回0，失败返回错误编号
```

## 同步属性

### 互斥锁属性
pthread_mutexattr_init 初始化 pthread_mutexattr_t，pthread_mutexattr_destroy 销毁 pthread_mutexattr_t
```c
#include <pthread.h>
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
返回值：成功返回0，失败返回错误码
```

pthread_mutexattr_t 支持 进程共享属性、健壮属性、类型属性

#### 进程共享属性
UNIX 允许相互独立的进程将同一个内存数据映射到各自的的地址空间，以这样的方式共享数据。

- 互斥锁的进程共享属性默认是 PTHREAD_PROCESS_PRIVATE ，表示互斥锁不支持跨进程操作。
- 将互斥锁的进程共享属性设置为 PTHREAD_PROCESS_SHARED ，那么在共享内存中创建互斥锁可以用于跨进程操作

pthread_mutexattr_getpshared 和 pthread_mutexattr_setpshared 用于设置和访问进程共享属性
```c
#include <pthread.h>
int pthread_mutexattr_getpshared(const pthread_mutexattr_t *restrict attr, int *restrict pshared);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
返回值：成功返回0，失败返回错误编号
```

#### 健壮属性
健壮属性和跨进程共享的互斥锁相关，默认为 PTHREAD_MUTEX_STALLED ，也可以被设置为 PHTREAD_MUTEX_ROBUST
- PTHREAD_MUTEX_STALLED 表示持有并锁定互斥锁的进程(或线程)如果终止，其他进程(或线程)就无法解锁并再使用这个互斥锁。这有可能导致死锁，意味着进程(线程)终止前必须将互斥锁解锁
- PHTREAD_MUTEX_ROBUST 表示持有并锁定互斥锁的进程(或线程)如果终止，其他进程(的线程)调用 pthread_mutex_lock 函数时返回 EOWNERDEAD，此时其他进程是可以解锁这个互斥锁的，但是解锁后就不能再锁定(调用lock函数会返回ENOTRECOVERABLE)了。如果需要锁定这个互斥锁，可以在解锁前调用 pthread_mutex_consistent 函数来恢复该锁的一致性
```c
#include <pthread.h>
int pthread_mutex_consistent(pthread_mutex_t * mutex);
返回值：成功返回0，失败返回错误编号
```

pthread_mutexattr_getrobust 和 pthread_mutexattr_setrobust 用于设置和获取健壮属性
```c
#include <pthread.h>
int pthread_mutexattr_getrobust(const pthread_mutexattr_t *restrict attr, int *restrict robust);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robust);
返回值：成功返回0，失败返回错误编号
```

#### 类型属性
互斥锁的类型和对应行为如下

| 互斥锁的类型  | 锁定已经锁定的互斥锁  | 锁定未占用的互斥锁  | 解锁已解锁的互斥锁  |
|:--|:--|:--|:--|
| PTHREAD_MUTEX_NORMAL  | 产生死锁  | 未定义  | 未定义  |
| PTHREAD_MUTEX_ERRORCHECK  | 返回错误  | 返回错误  | 返回错误  |
| PTHREAD_MUTEX_RECURSIVE  | 允许  | 返回错误  | 返回错误  |
| PTHREAD_MUTEX_DEFAULT  | 未定义或产生死锁  | 未定义  | 未定义  |
>备注：
>1. PTHREAD_MUTEX_DEFAULT 是互斥锁的默认类型，Linux将其映射为 PTHREAD_MUTEX_NORMAL
>2. 当互斥锁类型为 PTHREAD_MUTEX_RECURSIVE 时会维护锁定计数器，每被锁定一次，计数器都会自增。所以如果计数器未归零，互斥锁会仍处于锁定状态

pthread_mutexattr_gettype 和 pthread_mutexattr_settype 设置和获取类型属性
```c
#include <pthread.h>
int pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict attr, int *restrict type);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
返回值：成功返回0，失败返回错误编号
```

### 读写锁属性
pthread_rwlockattr_init 初始化读写锁属性，pthread_rwlockattr_destroy 销毁读写锁属性
```c
#include <pthread.h>
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
```

读写锁只支持 进程共享属性，属性值 PTHREAD_PROCESS_SHARED 和 PTHREAD_PROCESS_PRIVATE 的意义等同于互斥锁

使用 pthread_rwlockattr_getpshared 和 pthread_rwlockattr_setpshared 设置和获取该属性
```c
#include <pthread.h>
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *restrict attr, int *restrict pshared);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
返回值：成功返回0，失败返回错误码
```

### 条件变量属性
pthread_condattr_init 初始化 pthread_condattr_t， pthread_condattr_destroy 销毁 pthread_condattr_t
```c
#include <pthread.h>
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
返回值：成功返回0，失败返回错误码
```

pthread_condattr_t 支持 进程共享属性 和 时钟属性

#### 进程共享属性
进程共享属性与读写锁和互斥锁一致

pthread_condattr_getpshared 和 pthread_condattr_setpshared 设置和获取进程共享属性
```c
#include <pthread.h>
int pthread_condattr_getpshared(const pthread_condattr_t *restrict attr, int *restrict pshared);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
返回值：成功返回0，失败返回错误码
```

#### 时钟属性
时钟属性控制 pthread_cond_timedwait 函数使用哪种时钟计算超时

属性值是time.h中定义的时钟ID，包括
| 时钟ID | POSIX.1选项 | 描述 |
|:--|:--|:--|
|CLOCK_REALTIME|  | 实时系统时间 |
|CLOCK_MONOTONIC| _POSIX_MONOTONIC_CLOCK | 不带负跳数的实时系统时间 |
|CLOCK_PROCESS_CPUTIME_ID| _POSIX_CPUTIME | 调用进程的CPU时间 |
|CLOCK_THREAD_CPUTIME_ID| _POSIX_THREAD_CPUTIME | 调用线程的CPU时间 |

pthread_condattr_getpshared 和 pthread_condattr_setpshared 用于设置和获取时钟属性
```c
#include <pthread.h>
int pthread_condattr_getpshared(const pthread_condattr_t *restrict attr, clockid_t *restrict clock_id);
int pthread_condattr_setpshared(pthread_condattr_t *attr, clockid_t clock_id);
返回值：成功返回0，失败返回错误码
```
>备注：在使用 pthread_cond_timedwait 函数前，调用 pthread_cond_init 初始化 pthread_cond_t 时需要指定 pthread_condattr_t

### 屏蔽锁属性
pthread_barrierattr_init 和 pthread_barrierattr_destroy 用于创建和销毁屏蔽锁
```c
#include <pthread.h>
int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
返回值：成功返回0，失败返回错误码
```

屏蔽锁只支持进程共享属性， 使用 pthread_barrierattr_getpshared 和 pthread_barrierattr_setpshared 设置和获取 
```c
#include <pthread.h>
int pthread_barrierattr_getpshared(const pthread_condattr_t *restrict attr, int *restrict pshared);
int pthread_barrierattr_setpshared(pthread_condattr_t *attr, int pshared);
返回值：成功返回0，失败返回错误码
```

## 重入
线程是并行执行的，如果在同一时间点调用同一个函数，则有可能导致冲突，而如果一个函数在同一时间点可以被多个线程安全的调用，就称该函数是线程安全的。

支持线程安全函数的 UNIX 环境 (实现了 POSIX.1)会在 unistd.h 中宏定义 _POSIX_THREAD_SAFE_FUNCTIONS，sysconf 函数是查询 _SC_THREAD_SAFE_FUNCTIONS 也可以判断 UNIX 环境是否支持线程安全函数。

POSIX.1 为一部分非线程安全函数提供了线程安全的替代版本，这些函数后缀 _r，表示是可重用了。

线程安全不代表异步信号安全，因为如果某逻辑线程和信号处理函数都调用某线程安全函数，当逻辑线程未解锁资源时信号出现，强制调用的信号处理函数就会因为二次加锁导致锁死

### POSIX.1 中部分非线程安全的函数以及部分对应的线程安全版本
- 非线程安全的函数
basename catgets crypt dbm_clearerr dbm_close dbm_delete dbm_error dbm_fetch dbm_firstkey dbm_nextkey dbm_open dbm_store dirname dlerror drand48 encrypt endgrent endpwent endutxent getc_unlocked

getchar_unlocked getdate getenv getgrent getgrgid getgrnam gethostent getlogin getnetbyaddr getnetbyname getnetent getopt getprotobyname getprotobynumber getprotoent getpwent getpwnam getpwuid getservbyname getservbyport

getservent getutxent getutxid getutxline gmtime hcreate hdestroy hsearch inet_ntoa l64a lgamma lgammaf lgammal localeconv localtime lrand48 mrand48 nftw nl_langinfo ptsname

putc_unlocked putchar_unlocked putenv pututxline rand readdir setenv setgrent setkey setpwent setutxent strerror strsignal strtok system ttyname unsetenv wcstombs wctomb

- 部分对应的线程安全版本
getgrgid_r getgrnam_r getlogin_r getpwnam_r getpwuid_r gmtime_r localtime_r readdir_r strerror_r strtok_r ttyname_r

## 线程指定数据
也称为线程私有数据。虽然线程模型轻量级的共享数据方式非常方便，但是线程仍然有维护其私有数据的必要，如线程需要维护自己的errno，以免因重置进程的errno影响其他线程。
>备注：除了使用内核提供的寄存器机制，任何UNIX并不能保证线程私有数据被其他线程访问，线程私有数据机制仅仅是提高了线程间数据的独立性，并提高了访问其他线程私有数据的难度

### 使用线程指定数据
1. 分配线程指定数据之前，需要使用 pthread_key_create 函数初始化与该数据关联的键
```c
#include <pthread.h>
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
返回值：成功返回0，失败返回错误码
```
pthread_key_create 为 key关联了一个函数用来释放线程指定数据所在的内存(线程通常使用 malloc 为线程指定数据分配内存)，这个函数会在线程结束时被调用。

如果调用了多次 pthread_key_create 函数，那么线程结束时，系统会按照注册 destructor
 的顺序调用 destructor。所有 destructor 都执行完毕，系统会检查是否还有未清理的线程指定数据，如果有，那么再调用一轮，直到全部清理完毕，或达到了 PTHREAD_DESTRUCTOR_ITERATIONS 宏定义的最大次数
 
PTHREAD_KEYS_MAX 宏是线程可维护其私有数据的最大数

>备注：通常使用 pthread_once 函数来确保键初始化逻辑只调用一次
```c
#include <pthread.h>
pthread_once_t initflag = PTHREAD_ONCE_INIT;
int pthread_once(pthread_once_t *initflag, void (*initfn)(void));
返回值：成功返回0，失败返回错误码
```
> - initflag 必须时全局变量或静态变量，而且被初始化为PTHREAD_ONCE_INIT
> - 如果每个线程都调用 pthread_once， 系统将保证 initfn 函数只被调用一次

2. pthread_setspecific 函数将键和线程指定数据绑定，pthread_getspecific 函数通过键获取线程指定数据
```c
#include <pthread.h>
void *pthread_getspecific(pthread_key_t key);
返回值：线程指定数据值，若没有值与该键关联，返回NULL
int pthread_setspecific(pthread_key_t key, const void *value);
返回值：成功返回0，失败返回错误码
```

3. pthread_key_delete 用于取消键与线程指定数据的关联关系
```c
#include <pthread.h>
int pthread_key_delete(pthread_key_t key);
返回值：成功返回0，失败返回错误码
```

## 取消选项
线程提供 可取消状态 和 可取消类型 属性，这两个属性影响 pthread_cancel 函数的行为

调用 pthread_cancel 只是给目标线程发送取消请求，并不等待线程终止。默认情况下，线程会在运行到某个取消点的时候检查是否由取消请求，如果有才执行取消逻辑
>备注：
> 1.这两个选线没有包含在 pthread_attr_t 结构中
> 2.POSIX.1 规定会引起阻塞的系统调用都是取消点，包括：
> accept aio_suspend clock_nanosleep close connect creat fcntl fdatasync fsync lockf mq_receive mq_send mq_timedreceive
> mq_timedsend msgrcv msgsnd msync nanosleep open openat pause poll pread pselect pthread_cond_timedwait pthread_cond_wait
> pthread_join pthread_testcancel pwrite read readv recv recvfrom recvmsg select sem_timedwait sem_wait send sendmsg
> sendto sigsuspend sigtimedwait sigwait sigwaitinfo sleep system tcdrain wait waitid waitpid write writev

### 可取消状态
线程默认的可取消状态是 PTHREAD_CANCEL_ENABLE；当设置为 PTHREAD_CANCEL_DISABLE 时，取消请求会被阻塞，直到可取消状态恢复 PTHREAD_CANCEL_ENABLE ，在下一个取消点执行取消逻辑

pthread_setcancelstate 函数用来将可取消状态设置为 state，并保存原先的设置到 oldstate
```c
#include <pthread.h>
int pthread_setcancelstate(int state, int *oldstate);
返回值：成功返回0，失败返回错误码
```

当不确定线程调用的某个函数是否有取消点时，可以用 pthread_testcancel 函数添加取消点
```c
#include <pthread.h>
void pthread_testcancel(void);
```
>备注：
>1.Linux 环境中，目前c库函数都不是取消点(因为pthread库和c库函数结合的不好),所以为了符合POSIX标准的要求，需要在需要作为取消点的系统调用前后都加入取消点
>2.如果无限循环的逻辑中没有取消点，需要在其中加入取消点

```c
while (!done) {
    pthread_testcancel();           /* A cancellation point */
    ...
    }
}

```

### 可取消类型
可取消类型默认为 PTHREAD_CANCEL_DEFERRED，表示延迟到取消点；可设置为 PTHREAD_CANCEL_ASYNCHRONOUS，表示可以在任意时间取消，不用延迟到取消点

pthread_setcanceltype 函数设置可取消类型为 type，并保存原先设置到 oldtype
```c
#include <pthread.h>
int pthread_setcanceltype(int type, int *oldtype);
返回值：成功返回0，失败返回错误码
```

## 线程和信号
信号的接收处理是进程内所有线程共享的，这导致进程在处理不同线程注册的信号时会发生冲突。解决这个问题，需要给不同的线程设置不同的信号掩码

pthread_sigmask 用于设置线程的信号集
```c
#include <pthread.h>
int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
返回值：成功返回0，失败返回错误码
```
- how 可以设置为
    - SIG_BLOCK 或者 0，将set添加到当前信号集
    - SIG_UNBLOCK 或者 1，将set从当前信号集移除
    - SIG_SETMASK 或者 2，将当前信号集替换为set
- oset 用于保存之前的设置

调用 sigwait 函数可以使线程阻塞，信号出现后返回
```c
#include <pthread.h>
int sigwait(const sigset_t *restrict set, int *restrict sig);
返回值：成功返回0，失败返回错误码
```
- set 参数指定 sigwait 函数等待的信号集
- 当 sigwait 函数返回后，sig 参数指出哪种信号出现

pthread_kill 函数用来给指定的线程发送信号
```c
#include <pthread.h>
int pthread_kill(pthread_t thread, int sig);
返回值：成功返回0，失败返回错误码
```
- sig可以指定为0，用来测试线程是否存在

## 线程和fork
fork 的时候，父进程某个线程的锁(互斥锁、读写锁)的状态会被子进程继承，但是子进程没办法知道其占有了哪些锁，所以此时子进程无法立即已锁定资源，需要清理锁的状态
>备注：fork 之后马上调用 exec，则不需要清理锁

可以先使用 pthread_atfork 函数用来注册清理函数，然后fork
```c
#include <pthread.h>
int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
```
- prepare 由父进程在 fork 子进程前调用，用于获取父进程定义的所有锁
- parent 在 fork 子进程后，返回前在父进程中调用，用于清理父进程中的锁
- child 在 fork 返回后在子进程中调用，用于清理子进程中的锁
- 可以多次调用 pthread_atfork 注册多组 fork处理函数，parent 和 child 函数按照其注册顺序调用，prepare 函数的调用顺序则和注册顺序相反

>备注：pthread_atfork 机制存在一下不足：
> - 没办法清理条件变量和屏障锁
> - 某些错误检查的互斥锁在子进程中清理时会产生错误
> - 递归互斥锁不能在子进程中清理，因为没办法确定该互斥锁被加锁的次数
> - 如果子进程只允许调用异步信号安全函数，则不可能在子进程中清理锁，因为清理锁的所有函数都不是异步信号安全的
> - 如果信号处理程序中调用了 fork，pthread_atfork 注册的 fork处理程序只能调用异步信号安全的函数

## 线程和I/O
多线程下所有线程共享文件描述符，所以需要使用 pread pwrite 函数，将文件操作原子操作化，这样就不会出现IO的冲突。