# tasklet

## 为什么需要 tasklet

bottom half 根据如何推迟执行可以分为两种：有具体时间要求的（对应linux kernel中的低精度timer和高精度timer）和没有具体时间要求的。

对于没有具体时间要求的 bottom half 又可以分成两种：

- 越快越好型
- 随遇而安型

在系统中，越快越好型的 bottom half 不应该太多，而且 tasklet 的 callback 函数不能执行时间过长，否则会产生进程调度延迟过大的现象，甚至是非常长而且不确定的延迟，对 real time 的系统会产生很坏的影响。

在 linux kernel 中，softirq 和 tasklet 属于“越快越好型”，workqueue 和 threaded irq handler 属于“随遇而安型”。

tasklet 和 softirq 相比，有两点好处：

- tasklet 可以动态分配，也可以静态分配，数量不限。
- 同一种 tasklet 在多个 cpu 上也不会并行执行，这使得程序员在撰写 tasklet function 的时候比较方便，减少了对并发的考虑（当然损失了性能）。

但是也不能毫无条件的选择 tasklet，对于性能需求较高的，还是建议选择 softirq，没有性能需求的，可以考虑使用 workqueue 来代替。

## tasklet 的基本原理

### tasklet_struct 结构体

tasklet_struct 结构体用来描述一个 tasklet：
```c
struct tasklet_struct
{
    struct tasklet_struct *next;
    unsigned long state;
    atomic_t count;
    void (*func)(unsigned long);
    unsigned long data;
}; 
```
- 每个 cpu 都会维护一个链表，将本 cpu 需要处理的 tasklet 管理起来，next 这个成员指向了该链表中的下一个 tasklet。
- func 和data 成员描述了该 tasklet 的 callback 函数，func 是调用函数，data 是传递给 func 的参数。
- state 成员表示该 tasklet 的状态，TASKLET_STATE_SCHED 表示该 tasklet 已经被调度到某个 CPU 上执行，TASKLET_STATE_RUN 表示该 tasklet 正在某个 cpu 上执行。
- count 等于0那么该 tasklet 是处于 enable 的，如果大于0，表示该 tasklet 是 disable 的。

### tasklet_disable 和 tasklet_enable 函数

内核同步的场景不需 disable 所有的 softirq 和 tasklet，而仅仅是 disable 该 tasklet，tasklet_disable 和 tasklet_enable 用于此场景。
```c
static inline void tasklet_disable(struct tasklet_struct *t)
{
    tasklet_disable_nosync(t);  /* 给 tasklet 的 count 加一 */
    tasklet_unlock_wait(t);     /* 如果该 tasklet 处于 running 状态，那么需要等到该 tasklet 执行完毕 */
    smp_mb();
}

static inline void tasklet_enable(struct tasklet_struct *t)
{
    smp_mb__before_atomic();
    atomic_dec(&t->count);      /* 给 tasklet 的 count 减一 */
} 
```

### 管理 tasklet

Linux 内核为每个 CPU 都维护两个 tasklet 链表，分别维护普通优先级的 tasklet 和高优先级的 tasklet。
```c
static DEFINE_PER_CPU(struct tasklet_head, tasklet_vec);        /* 维护普通优先级的 tasklet */
static DEFINE_PER_CPU(struct tasklet_head, tasklet_hi_vec);     /* 维护高优先级的 tasklet */
```

普通优先级的 tasklet 基于 TASKLET_SOFTIRQ softirq 实现，高优先级的 tasklet 基于 HI_SOFTIRQ 的 softirq 实现。

### 定义 tasklet

DECLARE_TASKLET 宏用于静态定义 tasklet
```c
#define DECLARE_TASKLET(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(0), func, data }

#define DECLARE_TASKLET_DISABLED(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(1), func, data } 
```
- DECLARE_TASKLET_DISABLED 定义的 tasklet 默认处于 disable

也可以用 tasklet_init 初始化一个指定的 tasklet_struct
```c
tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data);
```

### 调度 tasklet

tasklet_schedule 函数用户调度一个 tasklet 执行
```c
static inline void tasklet_schedule(struct tasklet_struct *t)
{
    if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))  /* 为 tasklet_struct 的 state 成员设置 TASKLET_STATE_SCHED bit，如果已经设置，则不执行调度 */
        __tasklet_schedule(t);  /* 执行调度 */
}
```

可以在多个不同的 context 下多次调度同一个 tasklet 执行（也可能来自多个cpu core），但实际上，因为设置了 TASKLET_STATE_SCHED bit，该 tasklet 只会被挂入一个指定 CPU 的 tasklet 队列中，并且只会被调度一次。

__tasklet_schedule 函数如下：
```c
void __tasklet_schedule(struct tasklet_struct *t)
{
    unsigned long flags;

    local_irq_save(flags);      /* 关闭当前 CPU 上所有中断，并保存 TASKLET_STATE_SCHED flag */
    t->next = NULL;
    *__this_cpu_read(tasklet_vec.tail) = t; /* 将 tasklet_struc t 挂入当前 cpu tasklet_vec 链表的尾部 */
    __this_cpu_write(tasklet_vec.tail, &(t->next)); /* 当前 cpu tasklet_vec 链表尾指针指向 t-> next */
    raise_softirq_irqoff(TASKLET_SOFTIRQ);  /* raise TASKLET_SOFTIRQ softirq */
    local_irq_restore(flags);   /* 重启当前 CPU 上所有中断，恢复 TASKLET_STATE_SCHED flag  */
} 
```

### tasklet handler 执行时机

执行 __tasklet_schedule 后，系统会在适合的时间点执行 tasklet callback。

由于 tasklet 基于 softirq，tasklet handler 也就是 softirq handler，执行时机大体如下：

- 从中断（top half）返回用户空间进程上下文，有 pending 的 softirq handler 则执行。
- 内核空间进程上下文的代码离开 local_bh_disable/enable 临界区时，有 pending 的 softirq handler 则执行。
- 系统不断触发 softirq 中断或 bottom half 优先级高，则推迟到 softirqd daemon 中调度。

```c
static void tasklet_action(struct softirq_action *a)
{
    struct tasklet_struct *list;

    local_irq_disable();                        /* 关闭当前本地 CPU 所有中断，保证下面操作不会被打断 */
    list = __this_cpu_read(tasklet_vec.head);   /* 取出 tasklet_vec 链表，放在 list 中 */
    __this_cpu_write(tasklet_vec.head, NULL);   /* 重置 tasklet_vec 链表 */
    __this_cpu_write(tasklet_vec.tail, this_cpu_ptr(&tasklet_vec.head));
    local_irq_enable();                         /* 重启当前本地 CPU 所有中断 */

    while (list) {                          /* 遍历 tasklet 链表 */
        struct tasklet_struct *t = list;

        list = list->next;

        if (tasklet_trylock(t)) {           /* 为 tasklet 的 state 设置 TASKLET_STATE_RUN bit，已经设置，则不执行 tasklet handler */
            if (!atomic_read(&t->count)) {  /* count 等于 0 说明该 tasklet 是处于 enable 的，可以被调度执行了 */
                if (!test_and_clear_bit(TASKLET_STATE_SCHED, &t->state))    /* 清除 TASKLET_STATE_SCHED bit，如果已经被清除，那就有问题了 */
                    BUG();
                t->func(t->data);           /* 执行 tasklet handler */
                tasklet_unlock(t);          /* 清除 TASKLET_STATE_RUN 标记 */
                continue;                   /* 处理下一个 tasklet */
            }
            tasklet_unlock(t);              /* 清除 TASKLET_STATE_RUN 标记 */
        }

        local_irq_disable();                /* 如果该 tasklet 已经在别的cpu上执行，就将其挂入当前 CPU 的 tasklet 链表的尾部，这样在下一个 tasklet 执行时机到来的时候，kernel 会再次尝试执行该 tasklet */
        t->next = NULL;
        *__this_cpu_read(tasklet_vec.tail) = t;
        __this_cpu_write(tasklet_vec.tail, &(t->next));
        __raise_softirq_irqoff(TASKLET_SOFTIRQ);        /* 再次触发 softirq，等待下一个执行时机 */
        local_irq_enable();
    }
} 
```

```c
static inline int tasklet_trylock(struct tasklet_struct *t)
{
    return !test_and_set_bit(TASKLET_STATE_RUN, &(t)->state);
} 
```