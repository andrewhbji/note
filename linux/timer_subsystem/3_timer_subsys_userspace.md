# 时间子系统用户空间接口函数

## 前言

从应用程序的角度看，内核需要提供的和时间相关的服务有三种：

1. 和系统时间相关的服务。例如，在向数据库写入一条记录的时候，需要记录操作时间（何年何月何日何时）。
2. 进程睡眠。
3. 和 timer 相关的服务。在一段指定的时间过去后，kernel 要 alert 用户进程。

## 和系统时间相关的服务

### 秒级别的时间函数：time 和 stime

```c
#include <time.h>
time_t time(time_t *t);
int stime(time_t *t);
```
- time 获取当前时间点到 linux epoch 的秒数（内核中 timekeeper 模块保存了这个值， timekeeper->xtime_sec ）
- stime 是设置当前时间点到 linux epoch 的秒数。对于 linux kernel ，设置时间的进程必须拥有 CAP_SYS_TIME 的权限，否则会失败。

>备注：
> 1. linux kernel 通过 sys_time和 sys_stime 系统调用来支持这两个函数， 引入更高精度的时间后，这两个系统调用通过 sys_gettimeofday 和 sys_settimeofday 系统调用来实现 time 和 stime 函数。 内核中定义了 __ARCH_WANT_SYS_TIME 才可用使用 sys_time 和 sys_stime 系统调用。提供这两个的系统调用多半为了兼容旧的应用软件。
> 2. 配合上面的接口函数还有一系列将当前时间点到 linux epoch 的秒数转换成适合人类阅读的接口函数，例如 asctime ， ctime ， gmtime， localtime ， mktime ， asctime_r ， ctime_r ， gmtime_r ， localtime_r ，这些函数主要用来将 time_t 类型的时间转换成 break-down time或者字符形式。 

### 微秒级别的时间函数：gettimeofday 和 settimeofday

```c
#include <sys/time.h>
int gettimeofday(struct timeval *tv, struct timezone *tz);
int settimeofday(const struct timeval *tv, const struct timezone *tz);
```
- gettimeofday 函数获取从 linux epoch 到当前时间点的秒数以及微秒数（在内核态，这个时间值仍通过 timekeeper 模块获得的，具体接口是 getnstimeofday64 ，该接口的时间精度是纳秒级别的，通过除以 1000 调整到微秒级别的精度）。
- settimeofday 则是设置从 linux epoch 到当前时间点的秒数以及微秒数，设置时间的进程必须拥有 CAP_SYS_TIME 的权限，否则会失败。
- tz 参数是由于历史原因而存在，实际上内核并没有对 timezone 进行支持。

    >备注：
    > - gettimeofday 和 settimeofday 通过 sys_gettimeofday 和 sys_settimeofday 系统调用来实现
    > - 在新的 POSIX 标准中 gettimeofday 和 settimeofday 接口函数被标注为 obsolescent ，取而代之的是 clock_gettime 和 clock_settime 接口函数

### 纳秒级别的时间函数：clock_gettime 和 clock_settime

```c
#include <time.h>
int clock_getres(clockid_t clk_id, struct timespec *res);
int clock_gettime(clockid_t clk_id, struct timespec *tp);
int clock_settime(clockid_t clk_id, const struct timespec *tp); 
```
- clock_getres 用于获取系统时钟的精度
- clk_id 识别 system clock （系统时钟）的 ID ，定义如下：
    ```
    CLOCK_REALTIME              //  真实世界的时钟，允许人为设置和通过NTP协议调整
    CLOCK_MONOTONIC             // 禁止人为设置的真实世界时钟，但可以通过 NTP 协议调整
    CLOCK_MONOTONIC_RAW         //  是完全基于本地晶振的时钟,不能设置，也不能对晶振频率进行调整
    CLOCK_PROCESS_CPUTIME_ID    //  基于进程的系统时钟
    CLOCK_THREAD_CPUTIME_ID     //  基于线程的系统时钟
    ```
       
    >备注：
    > 1. CLOCK_MONOTONIC 一般会把系统启动的时间点设置为其基准点。随后该时钟会不断的递增。
    > 2. 尽量不要用 CLOCK_REALTIME 为某个动作设置定时器，因时间被修改导致定时器不能触发
    > 3. 系统启动时间一般使用 NONOTONIC 类型的时钟
    > 4. CLOCK_PROCESS_CPUTIME_ID 和 CLOCK_THREAD_CPUTIME_ID 一般用于各个应用程序的性能分析和统计，使用前需要先获取 clock id

        ```c
        #include <time.h>
        #include <pthread.h>
        #include <time.h>
        int clock_getcpuclockid(pid_t pid, clockid_t *clock_id);
        int pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id);
        ```
        
### 系统时钟的调整

Linux内核提供了 sys_adjtimex 系统调用为 adjtime 和 adjtimex 函数提供支持， sys_adjtimex 系统调用实现了 RFC 1305 定义的时间调整算法

adjtime 根据delta参数缓慢的修正 CLOCK_REALTIME 系统时钟
```c
int adjtime(const struct timeval *delta, struct timeval *olddelta);
```
- olddelta 返回上一次调整中尚未完整的 delta。

```c
#include <sys/timex.h>
int adjtimex(struct timex *buf);
```

## 进程睡眠

以下 sleep 函数都通过内核的 sys_nanosleep 系统调用实现（底层是基于 hrtimer ）

### 秒级别的 sleep 函数： sleep

阻塞指定时间后继续执行程序。此函数基于 CLOCK_REALTIME 时钟。
```c
#include <unistd.h>
unsigned int sleep(unsigned int seconds); 
```
- 到期返回 0，未到期时则返回因信号中断打断而剩余时间


### 微秒级别的 sleep 函数： usleep

该函数已被废弃，不建议使用

```c
#include <unistd.h>
int usleep(useconds_t usec); 
```
- 返回0表示执行成功，返回-1说明执行失败，错误码在 errno 中获取


### 纳秒级别的 sleep 函数： nanosleep

取代 usleep 函数
```c
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem); 
```
- 到期返回 0，因捕获信号而中断调用或失败返回 -1
- 如果 rem 不为空，恢复后记录剩余的时间，否则不记录


### 更高级的 sleep 函数： clock_nanosleep
```c
#include <time.h>
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain); 
```
- clock_id 指定计算 sleep 时间基于的时钟
- flag 等于 0 或者 1，分别指明 request 参数设置的时间值是相对时间还是绝对时间。

## 和 timer 相关的服务

### alarm 函数

alarm 函数基于 CLOCK_REALTIME 时钟实现定时器，到期后向调用进程发送 SIGALRM 信号。
```c
#include <unistd.h>
unsigned int alarm(unsigned int seconds); 
```

### Interval timer 函数

```c
#include <sys/time.h>
int getitimer(int which, struct itimerval *curr_value);
int setitimer(int which, const struct itimerval *new_value,
              struct itimerval *old_value); 
```
- getitimer 函数获取当前的 Interval timer 的状态，其中的 it_value 成员可以得到当前时刻到下一次触发点的世时间信息， it_interval 成员保持不变，除非你重新调用 setitimer 重新设置。
- old_value 返回上次 setitimer 函数的设置值。
- which 参数指定使用那种 timer （三选一），包括：

    - ITIMER_REAL ：基于 CLOCK_REALTIME 计时，超时后发送 SIGALRM 信号，和 alarm 函数一样。 
    - ITIMER_VIRTUAL ：只有当该进程的用户空间代码执行的时候才计时，超时后发送 SIGVTALRM 信号。 
    - ITIMER_PROF ：只有该进程执行的时候才计时，不论是执行用户空间代码还是陷入内核执行（例如系统调用），超时后发送 SIGPROF 信号。 
    
- itimerval 结构体可以用来指定定时器循环触发的时间，定义如下：
    ```c
    struct itimerval {
        struct timeval it_interval; /* next value */
        struct timeval it_value;    /* current value */
    };
    ```
    - it_value 指定 timer 首次触发的时间， timer 设置后会通过不断递减 it_value 来实现首次触发
    - it_interval 指定 timer 循环触发的间断时间。当 timer 触发后， it_interval 会赋值给 it_value ，通过不断递减 it_value 来实现循环触发
    
在新的 POSIX 标准中，这些函数已经过期，被 POSIX timer 函数取代。

### POSIX timer 函数

Interval timer 函数有以下不足：

- ITIMER_REAL、ITIMER_VIRTUAL 和 ITIMER_PROF 只能三选一
- 超时处理永远是用信号的方式，而且发送的 signal 不能修改。当 mask 信号处理的时候，虽然 timer 多次超期，但是 signal handler 只会调用一次，无法获取更详细的信息。
- Interval timer 函数精度是微秒级别，精度有进一步提升的空间。

因此 POSIX 提供了更加灵活的 timer 函数。

#### 创建 timer
```c
#include <signal.h>
#include <time.h>
int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid); 
```
- timerid 指针指向创建的 timer
- sigevent 结构体定义如下：
    ```c
    union sigval {          /* Data passed with notification */
        int     sival_int;         /* Integer value */
        void   *sival_ptr;         /* Pointer value */
    };

    typedef struct sigevent {
        sigval_t sigev_value;
        int sigev_signo;
        int sigev_notify;
        union {
            int _pad[SIGEV_PAD_SIZE];
            int _tid;

            struct {
                void (*_function)(sigval_t);
                void *_attribute;    /* really pthread_attr_t */
            } _sigev_thread;
        } _sigev_un;
    } sigevent_t; 
    ```
    sigev_notify 定义了当 timer 超期后如何通知该进程，可以设置为：
    
    - SIGEV_NONE ：不需要异步通知，程序自己调用 timer_gettime 来轮询 timer 的当前状态 
    - SIGEV_SIGNA ：使用 sinal 这样的异步通知方式。发送的信号由 sigev_signo 定义。如果发送的是 realtime signal ，该信号的附加数据由 sigev_value 定义。
    - SIGEV_THREAD ：创建一个线程执行 timer 超期 callback 函数， _attribute 定义了该线程的属性。
    - SIGEV_THREAD_ID ：行为和 SIGEV_SIGNAL 类似，不过发送的信号被送达进程内的一个指定的 thread ，这个 thread 由 _tid 标识。
    

#### 设置 timer
```c
#include <time.h>
int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec * old_value);
int timer_gettime(timer_t timerid, struct itimerspec *curr_value);
```
- flag 等于 0 或者 1 ，分别指明 new_value 参数设定的时间值是相对时间还是绝对时间。
- new_value.it_value 是一个非 0 值，那么调用 timer_settime 可以启动该 timer。如果 new_value.it_value 是一个 0 值，那么调用 timer_settime 可以 stop 该 timer。 

#### 删除 timer

删除指定的timer，释放资源。
```c
#include <time.h>
int timer_delete(timer_t timerid); 
```