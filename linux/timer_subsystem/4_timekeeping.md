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
- shadow_timekeeper 主要用在更新系统时间的过程中。在 update_wall_time 中，首先将时间调整值设置到 shadow_timekeeper 中，然后一次性的 copy 到真正的那个 timekeeper 中。这样的设计主要是可以减少持有 timekeeper_seq 锁的时间（在更新系统时间的过程中），不过需要注意的是：在其他的过程中（非 update_wall_time ），需要 sync shadow timekeeper 。 

## timekeeping 初始化

timekeeping 初始化的代码位于 timekeeping\_init函数中，在系统初始化的时候（ start_kernel ）会调用该函数进行 timekeeping 的初始化。

### 从 persistent clock 获取当前的时间值

timekeeping 模块中支持若干种 system clock ，这些 system clock 的数据保存在ram 中，一旦断电，数据就丢失了。因此，在系加电启动后，会从 persistent clock 中取出当前时间值（例如 RTC，RTC 由 battery 供电，因此系统断电也可以保存数据 ），根据情况初始化各种 system clock 。具体代码如下：
```c
    read_persistent_clock(&now);            /* 从系统中的 HW clock（例如RTC）中获取时间信息 */
    if (!timespec_valid_strict(&now)) {     /* 校验 now 的 sec 和 nsec 是否是有效 */
        now.tv_sec = 0;                     /* 不有效则将 sec 和 nsec 设置为 0 */
        now.tv_nsec = 0; 
    } else if (now.tv_sec || now.tv_nsec) 
        persistent_clock_exist = true;      /* persistent_clock_exist = true 表示系统存在 RTC 硬件模块，timekeeping 模块会和 RTC 模块进行交互 */
    read_boot_clock(&boot);
    if (!timespec_valid_strict(&boot)) { 
        boot.tv_sec = 0; 
        boot.tv_nsec = 0; 
    }
```

### 为 timekeeping 模块设置缺省的 clocksource

```c
clock = clocksource_default_clock();    /* 先选择已经就绪的 clocksource 为缺省 clocksource */
if (clock->enable) 
    clock->enable(clock);               /* enalbe default clocksource */ 
tk_setup_internals(tk, clock);          /* 设置默认 clocksource 和 timekeeping 之间的关系 */
```

### 初始化 real time clock ， monotonic clock 和 monotonic raw clock

```c
tk_set_xtime(tk, &now);         /* 根据 RTC 时间来初始化 timekeeping 中的 real time clock */
tk->raw_time.tv_sec = 0;        /* monotonic raw clock 被设置为从 0 开始 */ 
tk->raw_time.tv_nsec = 0; 
if (boot.tv_sec == 0 && boot.tv_nsec == 0) 
    boot = tk_xtime(tk);        /* 如果没有获取到有效的 booting time ，那么就选择当前的 real time clock */
set_normalized_timespec(&tmp, -boot.tv_sec, -boot.tv_nsec); /* 将 real time clock 只取负数作为 offset */ 
tk_set_wall_to_mono(tk, tmp);   /* 将 real time clock + offset 设置为 monotonic clock */
tmp.tv_sec = 0; 
tmp.tv_nsec = 0; 
tk_set_sleep_time(tk, tmp); /* 初始化 sleep time 为 0 */
```

## 获取和设置当前系统时钟的时间值

### 获取 monotonic clock 的时间值： ktime_get 和 ktime_get_ts

timekeeping 模块在 tick 到来的时候更新各种系统时钟的时间值， ktime_get 调用很有可能发生在两次 tick 之间，这时候，仅仅依靠当前系统时钟的值精度就不甚理想了，毕竟那个时间值是 per tick 更新的。因此，为了获得高精度，ns 值的获取是通过 timekeeping_get_ns 完成的，该函数获取了real time clock 的当前时刻的纳秒值，而这是通过上一次的 tick 时候的real time clock 的时间值（xtime_nsec）加上当前时刻到上一次 tick 之间的delta时间值计算得到的。
```c
ktime_t ktime_get(void) 
{ 
    struct timekeeper *tk = &timekeeper; 
    unsigned int seq; 
    s64 secs, nsecs;
    do { 
        seq = read_seqcount_begin(&timekeeper_seq); 
        secs = tk->xtime_sec + tk->wall_to_monotonic.tv_sec;            /* 获取 monotonic clock 的秒值 */
        nsecs = timekeeping_get_ns(tk) + tk->wall_to_monotonic.tv_nsec; /* 获取纳秒值 */
    } while (read_seqcount_retry(&timekeeper_seq, seq)); 
     
    return ktime_add_ns(ktime_set(secs, 0), nsecs);                     /* 返回一个ktime 类型的时间值 */
}
```

ktime_get_ts 的概念和 ktime_get 是一样的，只不过返回的时间值格式不一样

### 获取 real time clock 的时间值： ktime_get_real 和ktime_get_real_ts

这两个函数的具体逻辑动作和获取 monotonic clock 的时间值函数是完全一样的。这里稍微提一下另外一个函数： current_kernel_time ，代码如下：
```c
static inline struct timespec tk_xtime(struct timekeeper *tk) 
{ 
    struct timespec ts;
    ts.tv_sec = tk->xtime_sec; 
    ts.tv_nsec = (long)(tk->xtime_nsec >> tk->shift); 
    return ts; 
}
struct timespec current_kernel_time(void) 
{ 
    struct timekeeper *tk = &timekeeper; 
    struct timespec now; 
    unsigned long seq;
    do { 
        seq = read_seqcount_begin(&timekeeper_seq);
        now = tk_xtime(tk); 
    } while (read_seqcount_retry(&timekeeper_seq, seq));
    return now; 
}
```

上面的代码并没有调用 clocksource 的 read 函数获取 tick 之间的 delta 时间值，因此 current_kernel_time 是一个粗略版本的 real time clock ，精度低于 ktime_get_real ，不过性能要好些。类似的， monotonic clock 也有一个 get_monotonic_coarse 函数，概念类似 current_kernel_time 。

### 获取 boot clock 的时间值： ktime_get_boottime 和 get_monotonic_boottime

```c
ktime_t ktime_get_boottime(void) 
{ 
    struct timespec ts;
    get_monotonic_boottime(&ts); 
    return timespec_to_ktime(ts); 
}
```
boot clock 这个系统时钟和 monotonic clock 有什么不同？ monotonic clock 是从一个固定点开始作为 epoch，对于 linux ，就是启动的时间点，因此， monotonic clock 是一个从 0 开始增加的 clock ，并且不接受用户的 setting ，看起来好象适合 boot clock 是一致的，不过它们之间唯一的差别是对系统进入 suspend 的处理，对于 monotonic clock ，它是不记录系统睡眠时间的，因此 monotonic clock 得到的是一个 system uptime 。而 boot clock 计算睡眠时间，直到系统 reboot 。
ktime_get_boottime 返回 ktime 的时间值， get_monotonic_boottime 函数返回 timespec 格式的时间值。

### 获取 TAI clock 的时间值： ktime_get_clocktai 和 timekeeping_clocktai

原子钟和 real time clock（ UTC ）是类似的，只是有一个偏移而已，记录在 tai_offset 中。代码非常简单，大家自己阅读即可。ktime_get_clocktai 返回 ktime 的时间值，而 timekeeping_clocktai 返回 timespec 格式的时间值。

### 设置 wall time clock

```c
int do_settimeofday(const struct timespec *tv) 
{
……
    timekeeping_forward_now(tk);                    /* 更新 timekeeper 至当前时间 */
    xt = tk_xtime(tk); 
    ts_delta.tv_sec = tv->tv_sec - xt.tv_sec; 
    ts_delta.tv_nsec = tv->tv_nsec - xt.tv_nsec;    /* 计算 delta */
    tk_set_wall_to_mono(tk, timespec_sub(tk->wall_to_monotonic, ts_delta)); /* 不调mono clock */
    tk_set_xtime(tk, tv);                           /* 调整 wall time clock */
    timekeeping_update(tk, TK_CLEAR_NTP | TK_MIRROR | TK_CLOCK_WAS_SET); /* 更新 tk */
…… 
}
```
 
## 和 clocksource 模块的交互

除了直接调用 clocksource 的 read 函数之外， timekeeping 和 clocksource 主要的交互就是 change clocksource 的操作。当系统中有更高精度的 clocksource 的时候，会调用 timekeeping_notify 函数通知 timekeeping 模块切换 clock source 。
```c
int timekeeping_notify(struct clocksource *clock) 
{ 
    struct timekeeper *tk = &timekeeper;
    if (tk->clock == clock)         /* 新的 clocksource 和旧的一样，不需要切换 */ 
        return 0; 
    stop_machine(change_clocksource, clock, NULL);  /* 停止所有 CPU 上的任务，只执行 change_clocksource 函数 */
    tick_clock_notify();            /* 通知 tick 模块  */
    return tk->clock == clock ? 0 : -1; 
}
```

```c
static int change_clocksource(void *data)
{
	struct timekeeper *tk = &timekeeper;
	struct clocksource *new, *old;
	unsigned long flags;

	new = (struct clocksource *) data;

	raw_spin_lock_irqsave(&timekeeper_lock, flags);
	write_seqcount_begin(&timekeeper_seq);

	timekeeping_forward_now(tk);
	if (!new->enable || new->enable(new) == 0) {
		old = tk->clock;
		tk_setup_internals(tk, new);        /* 设定新的 clocksource ,禁用旧的 clocksource */
		if (old->disable)
			old->disable(old);
	}
	timekeeping_update(tk, true, true);     /* 更新 timekeeping */

	write_seqcount_end(&timekeeper_seq);
	raw_spin_unlock_irqrestore(&timekeeper_lock, flags);

	return 0;
}
```

```c
static void tk_setup_internals(struct timekeeper *tk, struct clocksource *clock) 
{ 
    cycle_t interval; 
    u64 tmp, ntpinterval; 
    struct clocksource *old_clock;
    old_clock = tk->clock; 
    tk->clock = clock;              /* 更换为新的 clocksource */ 
    tk->cycle_last = clock->cycle_last = clock->read(clock); /* 更新 last cycle 值 */
    tmp = NTP_INTERVAL_LENGTH;      /* NTP interval 设定的纳秒数 */
    tmp <<= clock->shift; 
    ntpinterval = tmp;              /* 计算remainder 的时候会用到 */ 
    tmp += clock->mult/2; 
    do_div(tmp, clock->mult);       /* 将 NTP interval 的纳秒值转成新clocksource 的 cycle 值 */
    if (tmp == 0) 
        tmp = 1;
    interval = (cycle_t) tmp; 
    tk->cycle_interval = interval;  /* 设定新的 NTP interval 的 cycle 值 */
    tk->xtime_interval = (u64) interval * clock->mult;      /* 将 NTP interval 的 cycle 值转成 ns */
    tk->xtime_remainder = ntpinterval - tk->xtime_interval; /* 计算 remainder */
    tk->raw_interval = 
        ((u64) interval * clock->mult) >> clock->shift;     /* NTP interval 的 ns值 */
     if (old_clock) {               /* xtime_nsec 保存的是不是实际的 ns 值而是一个没有执行 shift 版本的 */
        int shift_change = clock->shift - old_clock->shift; 
        if (shift_change < 0)       /* 如果新旧的 shift 值不一样，那么当前的 xtime_nsec 要修正 */ 
            tk->xtime_nsec >>= -shift_change; 
        else 
            tk->xtime_nsec <<= shift_change; 
    } 
    tk->shift = clock->shift;       /* 更换新的 shift factor */
    tk->ntp_error = 0; 
    tk->ntp_error_shift = NTP_SCALE_SHIFT - clock->shift;
    tk->mult = clock->mult;         /* 更换新的 mult factor */ 
}
```

## 和 tick device 模块的接口

## timekeeping 模块的电源管理

###初始化

```c
static struct syscore_ops timekeeping_syscore_ops = { 
    .resume        = timekeeping_resume, 
    .suspend    = timekeeping_suspend, 
};
static int __init timekeeping_init_ops(void) 
{ 
    register_syscore_ops(&timekeeping_syscore_ops); 
    return 0; 
}
device_initcall(timekeeping_init_ops);
```

在系统初始化的过程中，会调用 syscore_ops 结构体 timekeeping_init_ops 来注册和 timekeeping 相关的 system core operations 。注册的这些 callback 函数会在系统 suspend 和 resume 的时候，在适当的时机执行（在 system suspend 过程中，syscore suspend 的执行非常的靠后，在那些普通的总线设备之后，对应的，system resume 过程中，非常早的醒来进入工作状态）。

### suspned 回调函数

```c
static int timekeeping_suspend(void) 
{ 
    struct timekeeper *tk = &timekeeper; 
    unsigned long flags; 
    struct timespec        delta, delta_delta; 
    static struct timespec    old_delta;
    read_persistent_clock(&timekeeping_suspend_time);   /* 记录 suspend 的时间点 */ 
    if (timekeeping_suspend_time.tv_sec || timekeeping_suspend_time.tv_nsec) 
        persistent_clock_exist = true;                  /* 标示系统存在 RTC 硬件 */
    raw_spin_lock_irqsave(&timekeeper_lock, flags); 
    write_seqcount_begin(&timekeeper_seq); 
    timekeeping_forward_now(tk);                        /* 休眠前再次更新 timerkeeper 的系统时钟数据 */
    timekeeping_suspended = 1;                          /* 标示系统即将进入 suspend 状态 */
    delta = timespec_sub(tk_xtime(tk), timekeeping_suspend_time);－－－－－－－（4） 
    delta_delta = timespec_sub(delta, old_delta); 
    if (abs(delta_delta.tv_sec)  >= 2) { 
        old_delta = delta; 
    } else { 
        timekeeping_suspend_time = 
            timespec_add(timekeeping_suspend_time, delta_delta); 
    }
    timekeeping_update(tk, TK_MIRROR);－－－－更新shadow timekeeper 
    write_seqcount_end(&timekeeper_seq); 
    raw_spin_unlock_irqrestore(&timekeeper_lock, flags);
    clockevents_notify(CLOCK_EVT_NOTIFY_SUSPEND, NULL);－－－－－－－－（5） 
    clocksource_suspend();      /* suspend 系统中所有的 clocksource 设备 */
    clockevents_suspend();      /* suspend 系统中所有的 clockevent 设备 */
    return 0; 
}
```

### resume 回调函数

```c
static void timekeeping_resume(void) 
{ 
    struct timekeeper *tk = &timekeeper; 
    struct clocksource *clock = tk->clock; 
    unsigned long flags; 
    struct timespec ts_new, ts_delta; 
    cycle_t cycle_now, cycle_delta; 
    bool suspendtime_found = false;
    read_persistent_clock(&ts_new); －－－－－－通过 persistent clock 记录醒来的时间点
    clockevents_resume();－－－－－－－－－－－resume 系统中所有的clockevent设备 
    clocksource_resume(); －－－－－－－－－－resume 系统中所有的clocksource设备

    cycle_now = clock->read(clock); 
    if ((clock->flags & CLOCK_SOURCE_SUSPEND_NONSTOP) && 
        cycle_now > clock->cycle_last) {－－－－－－－－－－－－－－－－－－－－－－（1） 
        u64 num, max = ULLONG_MAX; 
        u32 mult = clock->mult; 
        u32 shift = clock->shift; 
        s64 nsec = 0;
        cycle_delta = (cycle_now - clock->cycle_last) & clock->mask; －－－本次suspend的时间 
        do_div(max, mult); 
        if (cycle_delta > max) { 
            num = div64_u64(cycle_delta, max); 
            nsec = (((u64) max * mult) >> shift) * num; 
            cycle_delta -= num * max; 
        } 
        nsec += ((u64) cycle_delta * mult) >> shift; －－－－将suspend时间从cycle转换成ns
        ts_delta = ns_to_timespec(nsec);－－－－将suspend时间从ns转换成timespec 
        suspendtime_found = true; 
    } else if (timespec_compare(&ts_new, &timekeeping_suspend_time) > 0) {－－－－－（2） 
        ts_delta = timespec_sub(ts_new, timekeeping_suspend_time); 
        suspendtime_found = true; 
    }
    if (suspendtime_found) 
        __timekeeping_inject_sleeptime(tk, &ts_delta); －－－－－－－－－－－－－－－－（3）
    tk->cycle_last = clock->cycle_last = cycle_now; －－－更新last cycle的值 
    tk->ntp_error = 0; 
    timekeeping_suspended = 0; －－－－标记完成了suspend/resume过程 
    timekeeping_update(tk, TK_MIRROR | TK_CLOCK_WAS_SET); －－更新shadow timerkeeper 
    write_seqcount_end(&timekeeper_seq); 
    raw_spin_unlock_irqrestore(&timekeeper_lock, flags);
    touch_softlockup_watchdog();
    clockevents_notify(CLOCK_EVT_NOTIFY_RESUME, NULL); －－－通知resume信息到clockevent 
    hrtimers_resume(); －－－高精度timer相关，另文描述 
}
```
（1）如果timekeeper当前的clocksource在suspend的时候没有stop，那么有机会使用精度更高的clocksource而不是persistent clock。前提是clocksource没有溢出，因此才有了cycle_now > clock->cycle_last的判断（不过，这里要求clocksource应该有一个很长的overflow的时间）。
（2）如果没有suspend nonstop的clock，也没有关系，可以用persistent clock的时间值。
（3）调用__timekeeping_inject_sleeptime函数，具体如下：
static void __timekeeping_inject_sleeptime(struct timekeeper *tk,  struct timespec *delta) 
{ 
    tk_xtime_add(tk, delta);－－－－－－将suspend的时间加到real time clock上去 
    tk_set_wall_to_mono(tk, timespec_sub(tk->wall_to_monotonic, *delta)); 
    tk_set_sleep_time(tk, timespec_add(tk->total_sleep_time, *delta)); 
    tk_debug_account_sleep_time(delta); 
}
monotonic clock不计sleep时间，因此wall_to_monotonic要减去suspend的时间值。total_sleep_time当然需要加上suspend的时间值。