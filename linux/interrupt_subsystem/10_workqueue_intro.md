# workqueue 的基本概念

## 为什么需要 workqueue

### 中断上下文为何不能 sleep？

linux 驱动工程师都知道：中断上下文（包括hard interrupt context和software interrupt context）中不能进行 sleep 操作。

所谓 sleep，就是调度器挂起当前的 task，然后在 run queue 中选择另外一个合适的 task 运行。

中断上下文是非常轻量级的，本身没有内核栈、内存空间等，但是中断上下文被调度时也需要这些资源，从哪里获取？当中断发生的那一时刻，中断上下文就直接使用当前进程的资源。

理论上，调度器是可以挂起中断上下文，并调度其他 task 进入 running 状态。但是，block 该中断上下文，也就 block 了为其提供资源的进程。

此外，来回在中断上下文、进程上下文、用户空间等之间切换本身需要较大的系统开销，所以这两点原因，block 一个中断上下文，调度其他 task，这样的行为系统开销太大，甚至会导致系统死机。

### 为什么需要 workqueue？

workqueue 和其他的 bottom half 最大的不同是它是运行在进程上下文中的，它可以睡眠，这和其他 bottom half 机制有本质的不同，大大方便了驱动工程师撰写中断处理代码。

此外，workqueue 和 kernel thread 相比，如果大量使用 kernel thread，也会影响整体性能，所以，workqueue 被选为更长用的 bottom half 实现机制。

## 2.6.x 时代的 workqueue

### 数据结构

workqueue_struct 结构体用于描述一个 workqueue，定义如下：
```c
struct workqueue_struct {
    struct cpu_workqueue_struct *cpu_wq; /* per-cpu work queue struct */
    struct list_head list; /* workqueue list */
    const char *name;
    int singlethread;   /* single thread or multi thread */
    int freezeable;     /* 和电源管理相关的一个flag */
}; 
```
- 一般情况下，创建一个 workqueue 时会为每一个 CPU 创建一个内核线程，如果设置 singlethread flag 就创建 single-threaded workqueue，在某些场景 per-cpu 的 worker thread 有些浪费。
- 当系统 suspend 时，所有用户空间的进程都会冻结，但是默认情况下内核线程是不能被冻结的。如果设置了 freezeable，该内核线程也可以冻结。

workqueue 就是一种把某些任务（work）推迟到一个或者一组内核线程中去执行，那个内核线程被称作 worker thread（每个 processor 上有一个 work thread）。系统中所有的 workqueue 会挂入一个全局链表，链表头定义如下：
```c
static LIST_HEAD(workqueues); 
```

cpu_workqueue_struct 结构体如下：
```c
struct cpu_workqueue_struct {

    spinlock_t lock; /* 用来保护worklist资源的访问 */

    struct list_head worklist;
    wait_queue_head_t more_work; /* 等待队列头 */
    struct work_struct *current_work; /* 当前正在处理的 work */

    struct workqueue_struct *wq; /* 指向 work queue struct */
    struct task_struct *thread; /* worker thread task */

    int run_depth;        /* Detect run_workqueue() recursion depth */
} ____cacheline_aligned; 
```
- worklist 时 CPU 的，因为每个 processor 都需要处理自己的 work
- work_struct 被挂入 worklist，可在 work 被调度时，处理 worklist 中每个 work_struct

work_struct 用于描述 work
```c
struct work_struct {
    atomic_long_t data;
    struct list_head entry;
    work_func_t func;
};

typedef void (*work_func_t)(struct work_struct *work);  
```
- func 指定具体要执行的 work 函数，这是个可被异步执行函数
- func 需要传入 work_struct 作为参数
- data 包含了该 work 的状态 flag 和挂入 workqueue 的信息

### 架构

![workqueue 架构](./workqueue.gif)

系统中包括若干的 workqueue，最著名的 workqueue 就是系统缺省的的 workqueue 了，定义如下：
```c
static struct workqueue_struct *keventd_wq __read_mostly; 
```

如果没有特别的性能需求，那么一般驱动使用 keventd_wq 就可以，毕竟没必要让系统创建太多的内核线程。

如果有必要，驱动模块可以创建自己的 workqueue。因此，系统中存在一个 workqueues 的链表，管理了所有的 workqueue 实例。一个 workqueue 对应一组 work thread（先不考虑 single thread 的场景），每个 cpu 一个，由 cpu_workqueue_struct 来抽象，这些 cpu_workqueue_struct 们共享一个 workqueue。

从底层驱动的角度来看，我们只关心如何处理 BH，驱动程序定义了 work_struct，其 func 成员就是 BH，然后挂入 worklist。驱动程序可以选择系统缺省的 workqueue，也可以自己创建一个workqueue，并把 work 挂入其中。work 在哪个 CPU 上调度，就挂入哪个 worker thread。

### workqueue API

#### 创建 work_struct

DECLARE_WORK 宏用于静态定义 work_struct
```c
#define DECLARE_WORK(n, f)                    \
    struct work_struct n = __WORK_INITIALIZER(n, f)

#define DECLARE_DELAYED_WORK(n, f)                \
    struct delayed_work n = __DELAYED_WORK_INITIALIZER(n, f) 
```

 INIT_WORK 宏用于创建指定的 work_struct
 ```c
 #define INIT_WORK(_work, _func)                        \
    do {                                \
        (_work)->data = (atomic_long_t) WORK_DATA_INIT();    \
        INIT_LIST_HEAD(&(_work)->entry);            \
        PREPARE_WORK((_work), (_func));                \
    } while (0) 
 ```

#### 调度 work
 
schedule_work 和 queue_work 都用于调度一个 work 执行，schedule_work 将 work 挂入缺省的系统 workqueue（keventd_wq），queue_work 将 work 挂入指定的 workqueue。
```c
int fastcall queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
    int ret = 0;

    if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work))) { /* 处于 pending 状态的 work 不会重复挂入 workqueue */
        __queue_work(wq_per_cpu(wq, get_cpu()), work);  /* 挂入 work list 并唤醒 worker thread */
        put_cpu();
        ret = 1;
    }
    return ret;
} 
```

wq_per_cpu 函数负责选择 workqueue 下的哪一个 cpu_workqueue_struct （也就是哪一个 worker thread）。一般情况下，根据当前的 cpu id，通过 per_cpu_ptr 获取 cpu_workqueue_struct 的数据结构，对于 single thread 而言，cpu 是固定的。
```c
static struct cpu_workqueue_struct *wq_per_cpu(struct workqueue_struct *wq, int cpu)
{
    if (unlikely(is_single_threaded(wq)))
        cpu = singlethread_cpu;
    return per_cpu_ptr(wq->cpu_wq, cpu);
} 
```

#### 创建 workqueue

create_workqueue 宏用于创建 workqueue
```c
#define create_workqueue(name) __create_workqueue((name), 0, 0)
#define create_freezeable_workqueue(name) __create_workqueue((name), 1, 1)
#define create_singlethread_workqueue(name) __create_workqueue((name), 1, 0)
```

```c
truct workqueue_struct *__create_workqueue(const char *name, int singlethread, int freezeable)
{
    struct workqueue_struct *wq;
    struct cpu_workqueue_struct *cwq;
    int err = 0, cpu;

    wq = kzalloc(sizeof(*wq), GFP_KERNEL);                      /* 分配 workqueue 的数据结构 */

    wq->cpu_wq = alloc_percpu(struct cpu_workqueue_struct);     /* 为每个 CPU 分配分配 worker thread 的内存 */

    wq->name = name;            /* 初始化 workqueue */
    wq->singlethread = singlethread;
    wq->freezeable = freezeable;
    INIT_LIST_HEAD(&wq->list);

    if (singlethread) { /* single thread workqueue 只选择一个 worker thread，由 singlethread_cpu 指定 */
        cwq = init_cpu_workqueue(wq, singlethread_cpu);         /* 初始化 cpu_workqueue_struct */
        err = create_workqueue_thread(cwq, singlethread_cpu);   /* 创建 worker thread */
        start_workqueue_thread(cwq, -1);            /* wakeup worker thread */
    } else {
        mutex_lock(&workqueue_mutex);
        list_add(&wq->list, &workqueues);   /* 将新创建的 workqueue 挂入 workqueues 全局列表 */

        for_each_possible_cpu(cpu) {        /* 在每一个 cpu 上创建了一个 worker thread 并通过 start_workqueue_thread 启动其运行 */
            cwq = init_cpu_workqueue(wq, cpu);
            if (err || !cpu_online(cpu))            /* 没有 online 的 cpu 就不需要创建 worker thread 了 */
                continue;
            err = create_workqueue_thread(cwq, cpu);
            start_workqueue_thread(cwq, cpu);
        }
        mutex_unlock(&workqueue_mutex);
    } 
    return wq;
} 
```

```c
static void start_workqueue_thread(struct cpu_workqueue_struct *cwq, int cpu)
{
    struct task_struct *p = cwq->thread;

    if (p != NULL) {
        if (cpu >= 0)
            kthread_bind(p, cpu);
        wake_up_process(p);
    }
} 
```
- create_workqueue 会在系统的每一个 CPU 上各创建一个工作队列, 以及一个 worker 线程, 被排入的工作在 worker 线程中执行
- create_freezeable_workqueue 和 create_singlethread_workqueue 都是创建 single thread workqueue，只不过一个是 freezeable 的，另外一个是 non-freezeable 的
- singlethread_cpu 在 init_workqueues 函数中进行初始化，但是 start_workqueue_thread(cwq, -1) 表示创建的 worker thread 并不绑定在任何的 cpu 上，调度器可以自由的调度该内核线程在任何的 cpu 上运行。
- 对于 single thread，kthread_bind 不会执行，对于普通的 workqueue，我们必须调用 kthread_bind 以便让 worker thread 在特定的 cpu 上执行。

#### work 执行的时机

work 执行的时机是和调度器相关的，当系统调度到 worker thread 这个内核线程后，该 thread 就会开始工作。每个 cpu 上执行的 worker thread 的内核线程的代码逻辑都是一样的，在 worker_thread 中实现： 
```c
static int worker_thread(void *__cwq)
{
    struct cpu_workqueue_struct *cwq = __cwq;
    DEFINE_WAIT(wait);

    if (cwq->wq->freezeable)    /* 如果是 freezeable 的内核线程，那么需要清除 task flag 中的 PF_NOFREEZE 标记，以便在系统 suspend 的时候冻结该 thread */
        set_freezable();        

    set_user_nice(current, -5); /* 提高进程优先级 */

    for (;;) {  /* worker thread 被调度后会一直运行，直到请求停止该 thread */
        prepare_to_wait(&cwq->more_work, &wait, TASK_INTERRUPTIBLE);
        /*
         * 导致 worker thread 进入 sleep 状态有三个条件:
         * 1. 电源管理模块没有请求冻结该 worker thread
         * 2. 该 thread 没有被其他模块请求停掉
         * 3. work list 为空，也就是说没有 work 要处理
         */
        if (!freezing(current) &&  !kthread_should_stop() &&  list_empty(&cwq->worklist))
            schedule();
        finish_wait(&cwq->more_work, &wait);

        try_to_freeze();            /* 处理来自电源管理模块的冻结请求 */

        if (kthread_should_stop())  /* 处理停止该 thread 的请求 */
            break;

        run_workqueue(cwq);         /* 依次处理 work list 上的各个 work */
    }

    return 0;
} 
```



