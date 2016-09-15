# 第七章 时间、延时和延迟工作

## 测量时间流动

## 获取当前时间

## 延后执行

## 内核定时器

## Tasklets 机制

## 工作队列

## 测量时间流动
内核依据硬件发出的计时器中断来追踪时间的流动, 计算机启动后, 内核依据 HZ 值(定义在 linux/param.h) 来设定每秒发出多少个计时器中断

内核维护 jiffies 和 jiffies_64 变量用来记录计时器中断次数(称为时间戳)

### jiffies
jiffies 定义在 linux/jiffies.h， 类型是 volatile(自变性)这个变量是只读的, 可以被直接访问, 每次访问都会获得当前时间戳
```c
#include <linux/jiffies.h>
unsigned long j, stamp_1, stamp_half, stamp_n;

j = jiffies; /* 获取当前的时间戳 */
stamp_1 = j + HZ; /* 一秒之后 */
stamp_half = j + HZ/2; /* 半秒之后 */
stamp_n = j + n * HZ / 1000; /* n 秒之后 */
```

jiffies 有溢位的可能(从最大值变为最小值), 在 HZ 被设定为 1000 的 32-bit 平台上, 大约连续开机 50 天左右, jiffies 就会发生一次溢位

内核提供以下宏可以在不用担心溢位的情况下比较两个时间戳之间的关系
```c
#include <linux/jiffies.h>
int time_after(unsigned long a, unsigned long b);
int time_before(unsigned long a, unsigned long b);
int time_after_eq(unsigned long a, unsigned long b);
int time_before_eq(unsigned long a, unsigned long b);
```
- after/before 用来判断 a 是否在 b 之后/之前, 结果为真则返回非零值
- *eq可以判断两个时间戳是否相等

内核提供一下函数用于 jiffies 和 timval, timespec 之间的转换
```c
#include <linux/time.h>
unsigned long timespec_to_jiffies(struct timespec *value);
void jiffies_to_timespec(unsigned long jiffies, struct timespec *value);
unsigned long timeval_to_jiffies(struct timeval *value);
void jiffies_to_timeval(unsigned long jiffies, struct timeval *value);
```

### jiffies_64
在64-bit 平台上, jiffies_64 和 jiffies 是同一个变量, 但是在32-bit 平台上要分两次访问操作才能得到完整的64-bit 值

内核提供原子操作函数 get_jiffies_64 用于获取 jiffies_64 值
```c
u64 get_jiffies_64(void);
```

### 时脉计数器
x86 平台上内建一个 TSC(TimeStamp Counter)缓存, 长度为64-bit, 用于记录CPU经过的时脉周期数

Linux 提供对 TSC 缓存的支持(asm/msr.h, 这个头文件专用于 x86平台, MSR表示 "Machine-Specific Registers"), 下面的宏用于获取TSC 缓存值
```c
rdtsc(low32, high32);
rdtecl(low32);
rdtscll(var64);
```
- 这几个宏都是原子操作
- *l 表示获取低半部, **ll表示获取全部
- rdtsc 可同时获取高半部和低半部
- TSC 主要用来测量小于毫秒的时间差, 这时只要获取低半部即可, 如果测量的时间差接近一秒, 最好获取完整的64-bit

其他平台如果也提供了时脉计数器, linux 则提供 get_cycles 函数用于跨平台操作
```c
#include <linux/timex.h>
cycles_t get_cycles(void);
```
- 返回0 表示目标平台没有时脉计数器

## 获取当前时间

jiffies 适合时间差超过1/HZ的时间间隔, TSC 时间适合时间差小于1/HZ的时间间隔

驱动程序一般没有使用墙上时间的需求, 如果有需求, 通常意味着操作规则被卸载驱动程序里, 这时需要考虑这样的设计是否有错, 通常情况下这属于策略的部分, 应当由用户空间的程序来处理

### 获取墙钟时间
linux 内核提供 mktime 函数用于将当前墙上时钟换算成"从1970/1/1 0:0:0(基准时刻)起经历的秒数", 一般将这个时间传递给用户空间的程序处理
```c
#include <linux/time.h> 
unsigned long mktime (unsigned int year, unsigned int mon, unsigned int day, unsigned int hour, unsigned int min, unsigned int sec); 
```

do_gettimeofday 函数用于获取当前绝对时间戳
```c
 #include <linux/time.h>
void do_gettimeofday(struct timeval *tv);
```

xtime 变量和current_kernel_time 函数:
xtime 变量可以精确到1/HZ, 不能直接使用 xtime 变量, 因为和jiffies一样, xtime 随时在变, 需要事先取得它的锁然后才能读取它的 tv_sec 和 tv_usec 成员, 为此, 内核提供 current_kernel_time 函数获取 xtime
```c
#include <linux/time.h>
struct timespec current_kernel_time(void);
```

## 延迟执行
选择延迟技术, 需要评估延迟时间与计时器中断之间的关系, 而且要将个平台的 HZ 的可能范围都考虑进去

### 长期延迟
长期延迟是指大于1个jiffies以上的计时器中断量

长期延迟使用技术包括:
- 忙等待循环
- 在循环中渡让处理器
- 休眠

#### 忙等待循环
忙等待循环一般为下面的形式
```c
#include <linux/jiffies.h>
unsigned long j1;

j1 = jiffies + HZ;  /* 1秒后的 jiffies 值 */

while (time_before(jiffies, j1)){
    cpu_relax();
}
```

cpu_relax 函数会占用 cpu 但是什么都不做

cpu 空闲时, 使用忙等待循环, 延迟时间符合预期, 执行总时间稍微大于内核执行时间

cpu 高负载时, 在非抢占式内核上使用忙等待循环, 延迟时间符合预期

cpu 高负载时, 在抢占式内核上使用忙等待循环, 延迟时间超过预期

#### 在循环中渡让处理器
渡让处理器一般为下面的形式
```c
#include <linux/jiffies.h>
unsigned long j1;

j1 = jiffies + HZ;  /* 1秒后的 jiffies 值 */

while (time_before(jiffies, j1)){
    schedule();
}
```

schedule 函数执行时, 会让执行单元让出处理器, 但仍然在运行队列中

cpu 空闲时, 在循环中渡让处理器, 延迟时间符合预期, 内核执行时间稍微大于执行总时间, 多出的时间是内核执行 scheduler 时调度进程所花的时间

cpu 高负载时, 在循环中渡让处理器, 延迟时间超过预期, 因为一旦将 cpu 让出, 无法预期什么时候重新获得 cpu

#### 休眠
Linux 休眠机制以完美解决前两种计数在高负载时的缺陷, 而且占用cpu时间最少

休眠的形式有两种
- 使用 wait_event_interruptible_timeout 和 wait_event_timeout 函数, 但会引入一个多余的等待队列
```c
#include <linux/jiffies.h>
unsigned long delay = HZ;
init_waitqueue_head (&wait);
wait_event_interruptible_timeout(wait, 0, delay);	/*  执行单元在等待队列wait上休眠delay指定的jiffies时间 */
```
- 使用 schedule_timeout 函数, 可以不必准备多余的待命队列
```c
#include <linux/jiffies.h>
unsigned long delay = HZ;
set_current_state(TASK_INTERRUPTIBLE);	/* 将执行单元状态设置为可中断的休眠状态 */
schedule_timeout (delay);	/* 重新排程, 效果是执行单元休眠delay个jiffies时间 */
```

### 短期延迟
短期延迟适合于小于1个jiffies以上的计时器中断量, 通常最多只有几十个微秒

linux 内核提供三种等级的短期延迟函数
```c
#include <linux/delay.h>
void ndelay(unsigned long nsecs);
void udelay(unsigned long usecs);
void mdelay(unsigned long msecs);
```
- 这些函数的使用原则都是宁可多延迟
- 这些函数的参数都是 long 类型, 但是实际传入的值最好不超过1000, 否则造成计算循环次数时发生溢位(udelay 和 ndelay 通过循环来实现延迟效果)
- 延迟期间, 系统不能执行其他任务

另外, linux 内核提供一组不适用忙等待循环的毫秒级(或更久)的延迟函数
```c
#include <linux/delay.h>
void msleep(unsigned int millisecs);
unsigned long msleep_interruptible(unsigned int millisecs);
void ssleep(unsigned int seconds)
```
- msleep_interruptible 允许被中断打断, 完成延迟则返回 0 , 中断延迟则返回剩余的毫秒数

## 内核定时器
内核定时器可用于安排函数在未来的特定时间点执行(以计时器中断数为单位), 也可用于检查硬件的状态(适用于无法触发中断的硬件), 或完成需要延迟的设备关机程序

内核定时器安排的函数实在软中断的原子环境中运行, 所以被安排函数需要遵循"自旋锁与原子环境"的限制

非进程环境里执行的程序(如在中断环境里), 需要遵循下面的限制
- 不允许访问用户空间
- 软中断环境下, current 指针是没有意义的
- 不允许休眠以及调用任何可能引起休眠的函数(如 kmalloc), 也不允许触发调度

in_interrupt 和 in_atomic 宏分别用来判断执行单元是否在中断环境或原子环境中
```c
#include <linux/hardirq.h>
in_interrupt()
in_atomic()
```
- 返回非0值代表执行单元在在中断环境或原子环境中

内核计时器可以被重新注册, 以便复用

内核计时器在SMP系统中总是在当初注册时的那个CPU上执行

内核计时器是潜在的竞态现象的来源, 因此计时器函数所访问的任何数据结构都应该被保护, 以免被同时访问

### 内核计时器API
timer_list 结构体用来描述计时器
```c
struct timer_list
{
        /* ... */
        unsigned long expires;
        void (*function)(unsigned long);
        unsigned long data;
};
```
- function 是指向被计时器安排执行的函数的指针
- expires 描述function被执行的时间, 值为 jiffies 的绝对值
- data 是要传给 function 的参数

timer_list 结构体使用 init_timer 和 TIMER_INITIALIZER 宏来初始化
```c
void init_timer(struct timer_list *timer);
struct timer_list TIMER_INITIALIZER(_function, _expires, _data);
```

add_timer 函数用来注册计时器
```c
void add_timer(struct timer_list * timer);
```

del_timer 和 del_timer_sync 函数注销计时器
```c
int del_timer(struct timer_list * timer);
int del_timer_sync(struct timer_list *timer);
```
- 在 SMP 系统上, 一般使用 del_timer_sync 来避免竞态现象, del_timer_sync 会等待其他处理器上的运行的定时器处理程序都退出
- del_timer_sync 在非原子环境下调用时, 可能会休眠, 在原子环境或中断环境下调用时, 会以忙等待循环来等待计时器函数结束

mod_timer 函数更新 timer_list 的 expires 参数, 即修改到期时间
```c
int mod_timer(struct timer_list *timer, unsigned long expires);
```

timer_pending 函数判断计时器函数是否已经准备好要被执行
```c
int timer_pending(const struct timer_list * timer);
```

## Tasklet 机制
Tasklet也是一种定时器, 主要用于中断管理

Tasklet不能被指定在特定的时间点执行, 只能将其送入调度系统, 然后由内核自助决定合适执行

Tasklet也是在软中断的原子环境下运行

### Tasklet API
tasklet_struct 结构体用来指定定时器处理函数
```c
#include <linux/interrupt.h> 
struct tasklet_struct {
 /* ... */

void (*func)(unsigned long);
 unsigned long data;
};
```

初始化 tasklet_struct 结构体
```c
tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data);
DECLARE_TASKLET(name, func, data);
DECLARE_TASKLET_DISABLED(name, func, data);
```

tasklet_schedule 和 tasklet_hi_schedule 函数将 tasklet 排入调度系统
```c
void tasklet_schedule(struct tasklet_struct *t);
void tasklet_hi_schedule(struct tasklet_struct *t);
```
- tasklet_hi_schedule 会以高优先级调度 taskket
- tasklet 在执行的时候, 可以将自己再调度一次(自我调度)

tasklet_disable 和 tasklet_enable 函数使 tasklet 失效/生效, 失效的 tasklet 仍可以被排入调度系统, 但是定时器不会生效
```c
void tasklet_disable(struct tasklet_struct *t);
void tasklet_disable_nosync(struct tasklet_struct *t);
void tasklet_enable(struct tasklet_struct *t);
```
- 如果 tasklet 正在执行, tasklet_disable 会等待当前 tasklet 执行完毕后返回, tasklet_disable_nosync 则会立即返回
- tasklet_enable 的调用次数必须与tasklet_disable的调用次数一致

tasklet_kill 杀死 tasklet, 使之不会再进入调度系统
```c
void tasklet_kill(struct tasklet_struct *t);
```
- 如果 tasklet 正在执行中, tasklet_kill 会等待其执行完毕后再返回
- 能够自我调度的 tasklet, 需要先设法阻止其自我调度, 然后再调用 tasklet_kill

## 工作队列
工作队列的一些特点:
- 排入工作队列的函数, 是由内核的一个特殊进程负责执行, 不想 tasklet 那样有很多限制条件, 甚至允许休眠
- 排入工作队列的函数默认是在当初注册的cpu上执行
- 工作队列可以指定执行间隔
- 工作队列里的函数可以有较长的延迟时间, 而且不必使用原子操作
- 被排入工作队列中的函数不能访问用户空间

### 工作队列API
工作队列使用 workqueue_struct 结构体描述

工作则由 work_struct 结构体描述

create_workqueue 函数创建工作队列
```c
struct workqueue_struct *create_workqueue(const char *name);
struct workqueue_struct *create_singlethread_workqueue(const char *name);
```
- create_workqueue 会在系统的每一个CPU上各创建一个工作队列, 以及一个 worker 线程, 被排入的工作在 worker 线程中执行
- create_singlethread_workqueue 只会产生一个工作队列和一个 worker 线程

destroy_workqueue 函数用于销毁工作队列

下面的宏用于初始化工作
```c
DECLARE_WORK(name, void (*function)(struct work_struct *work));
INIT_WORK(struct work_struct *work, void (*function)(struct work_struct *work));
```

queue_work 和 queue_delayed_work 函数用于将工作排入工作队列
```c
int queue_work(struct workqueue_struct *queue, struct work_struct *work);
int queue_delayed_work(struct workqueue_struct *queue, struct work_struct *work, unsigned long delay);
```
- delay 参数用于指定至少经过 delay 个jiffies后, 才会执行排入的 work

cancel_delayed_work 函数用于取消工作队列中等待的工作
```c
int cancel_delayed_work(struct work_struct *work);
```
- 成功取消工作则返回非0值, 失败则返回0, 失败时因为该工作已经在另一个不同的cpu上执行了

flush_workqueue 函数用于清空指定工作队列中的工作, 一般在 cancel_delayed_work 失败后调用
```c
void flush_workqueue(struct workqueue_struct *queue);
```

### 公共工作队列
使用内核预设的公共工作队列, 有助与提高效率

公共工作队列不能被独占使用的太久, 换言之排入队列的函数不能长期休眠, 还需要将预期工作函数的执行延迟时间设置的长一些

schedule_work 函数用于将工作排入公共工作队列
```c
int schedule_work(struct work_struct *work);
int schedule_delayed_work(struct work_struct *work, unsigned long delay);
```
- delay 参数用于指定至少经过 delay 个jiffies后, 才会执行排入的 work

flush_scheduled_work 函数用于清空公共工作队列中的工作
```c
void flush_scheduled_work(void);
```
