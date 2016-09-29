# timekeeping 模块

timekeeping 模块是一个提供时间服务的基础模块。Linux 内核提供各种 time line ， real time clock ， monotonic clock ， monotonic raw clock 等， timekeeping 模块负责跟踪、维护这些 timeline ，并且向其他模块（ timer 相关模块、用户空间的时间服务等）提供服务，而 timekeeping 模块维护 timeline 的基础是基于 clocksource 模块和 tick 模块。tick 模块通过触发 tick，周期性的更新 time line ，通过 clocksource 模块、可以获取 tick 之间更精准的时间信息。

## timekeeper 核心数据定义

### timekeeper 数据结构
struct timekeeper 结构体用来管理各种系统时钟的跟踪以及控制，定义如下： 
```c
struct timekeeper {
    struct clocksource    *clock;               /* 指定当前使用的 clocksource */
    
    u32            mult;                        /* 用于 clock source 的 cycle 值和纳秒转换的因数 */
    u32            shift;

    cycle_t            cycle_interval;          /* NTP 相关的成员 */
    cycle_t            cycle_last;
    u64            xtime_interval;
    s64            xtime_remainder;
    u32            raw_interval;

    s64            ntp_error;
    u32            ntp_error_shift;

    u64            xtime_sec;                   /* CLOCK_REALTIME 系统时钟 */
    u64            xtime_nsec;

    struct timespec        wall_to_monotonic;   /* CLOCK_MONOTONIC 系统时钟 */
    ktime_t            offs_real;     
    struct timespec        total_sleep_time;    /* 记录系统睡眠时间 */
    ktime_t            offs_boot;               /* 记录系统 boot time */
    
    struct timespec        raw_time;            /* CLOCK_MONOTONIC_RAW 系统时钟 */
   
    s32            tai_offset;                  /* CLOCK_TAI 系统时钟 */
    ktime_t            offs_tai;

}; 
```
- xtime_* 成员表示使用 CLOCK_REALTIME 系统时钟， xtime_sec 表示用秒来度量当前时间点， xtime_nsec 用来以纳秒度量的换算因数，当前时间点的值应该是 xtime_sec + (xtime_nsec << shift)。
- wall_to_monotonic 成员定义了 CLOCK_MONOTONIC 到 CLOCK_REALTIME 的偏移，也就是说，这里的 wall_to_monotonic 和 offs_real 需要加上 real time clock 的时间值才能得到 monotonic clock 的时间值。
- offs_real 和 wall_to_monotonic 的意思是一样，不过时间的格式不一样，用在不同的场合。
- TAI（international atomic time）是原子钟，一般之从 UTC 同步过来的时间。

### 全局变量
```c
static struct timekeeper timekeeper;
static DEFINE_RAW_SPINLOCK(timekeeper_lock);
static seqcount_t timekeeper_seq;

static struct timekeeper shadow_timekeeper; 
```
- timekeeper 维护了系统的所有的 clock。
- timekeeper_lock 和 timekeeper_seq 都是用来保护 timekeeper 的，用在不同的场合。

- shadow_timekeeper 主要用在更新系统时间的过程中。在 update_wall_time 中，首先将时间调整值设定到 shadow_timekeeper 中，然后一次性的 copy 到真正的那个 timekeeper 中。这样的设计主要是可以减少持有 timekeeper_seq 锁的时间（在更新系统时间的过程中），不过需要注意的是：在其他的过程中（非 update_wall_time ），需要 sync shadow timekeeper 。 

## timekeeping 初始化

## 获取和设定当前系统时钟的时间值

## 和 clocksource 模块的交互

## 和 tick device 模块的接口

## timekeeping 模块的电源管理