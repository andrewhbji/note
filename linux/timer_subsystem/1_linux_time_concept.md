# Linux 内核中和时间相关的基本概念

## 系统时钟

时间是线性的，除了要给出度量单位（例如：秒），还有给出参考点。对于 linux kernel 而言，这个参考点就是 Linux Epoch ， 即 UTC 时间（1970 年 1 月 1 日 0 点 0 分 0 秒） 。一个符合 POSIX （可移植操作系统标准） 的系统必须提供系统时钟，以不小于秒的精度来记录到 linux epoch 的时间值。

内核支持的 system clock 定义如下:
```c
#define CLOCK_REALTIME            0
#define CLOCK_MONOTONIC            1

…… 
```
- CLOCK_REALTIME 是描述真实世界的时钟
- CLOCK_MONOTONIC 是一个禁止人为设定的真实世界的时钟，你没有办法设定它，但是可以通过NTP协议进行调整

## broken-down POSIX time

在时间线上以 linux epoch 为参考点，用到参考点的秒数来表示时间线上的某个时间点的方法对于计算机而言当然是方便的，不过对于使用计算机的人类而言，我们更习惯 broken-down time ，也就是将当前时间到参考点的秒数值分解成年月日时分秒。在内核中用下面的数据结构表示：
```c
struct tm {
    /*
     * the number of seconds after the minute, normally in the range
     * 0 to 59, but can be up to 60 to allow for leap seconds
     */
    int tm_sec;
    /* the number of minutes after the hour, in the range 0 to 59*/
    int tm_min;
    /* the number of hours past midnight, in the range 0 to 23 */
    int tm_hour;
    /* the day of the month, in the range 1 to 31 */
    int tm_mday;
    /* the number of months since January, in the range 0 to 11 */
    int tm_mon;
    /* the number of years since 1900 以 NTP epoch 为基准点*/ 
    long tm_year;
    /* the number of days since Sunday, in the range 0 to 6 */
    int tm_wday;
    /* the number of days since January 1, in the range 0 to 365 */
    int tm_yday;
}; 
```

## 不同精度的时间表示

传统的unix使用了基于秒的时间定义，相关的数据结构是time_t：
```c
typedef long        time_t; 
```

time_t 是 POSIX 标准定义的一个以秒计的时间值。例如 time 函数，可以获取一个从 linux Epoch 到当前时间点的秒值，该函数的返回值类型就是 time_t 的。如果 time_t 被实现成一个 signed 32-bit integer ，那么实际上在 2038 年的时候就会有溢出问题。

随着应用的发展，秒精度已经无法满足要求，因此出现了微秒精度的时间表示：
```c
struct timeval { 
    time_t    tv_sec;        /* seconds */ 
    suseconds_t    tv_usec;    /* microseconds */
};
```

struct timeval 的概念和 time_t 是一样的，只不过多了一个微秒的成员，将时间的精度推进到微秒级别。在计算当前时刻到 epoch 时间点的微秒数值的时候可以使用公式： tv_sec x 10^6 + tv_usec 。

由于实时应用程序的需求，POSIX 标准最终将时间精度推进到纳秒，纳秒精度的时间表示如下：
```c
struct timespec { 
    time_t    tv_sec;            /* seconds */
    long      tv_nsec;        /* nanoseconds */
};
```

根据 POSIX 标准，时间线上一个时间点的值用 struct timespec 来表示，它应该最少包括： 

| 成员数据类型 | 成员的名字 | 描述 |
| :-- | :-- | :-- |
|  time_t | tv_sec |  	该时间点上的秒值，仅在大于或者等于 0 的时候有效。 |
| long | tv_nsec | 该时间点上的 ns 值，仅在大于或者等于 0，并且小于 10^9 纳秒的时候有效。|

struct timespec 和 struct timeval 概念类似。