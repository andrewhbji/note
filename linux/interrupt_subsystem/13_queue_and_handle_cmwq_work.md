# workqueue 如何处理 work

## 将 work 加入 workqueue

CMWQ 对外提供了 queue_work_on 函数用于将定义好的 work 加入 workqueue，

### 拒绝 pending 状态的 work 加入

queue_work_on 函数不允许处于 pending 状态的 work 再次加入 workqueue ，代码如下：
```c
bool queue_work_on(int cpu, struct workqueue_struct *wq, struct work_struct *work)
{
    ……
    
    /* 处于 pending 状态的 work 不会被加入 workqueue */
    if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work))) {
        __queue_work(cpu, wq, work);        /* 加入 work list 并通知 worker pool 来处理 */
        ret = true;
    }

    ……
} 
```
- work_struct 的 data 成员中设置了 WORK_STRUCT_PENDING_BIT 标识了该 work 是处于 pending 状态还是正在处理中。

具体将 work 加入 workqueue 在 __queue_work 函数中进行

### workqueue 销毁时拒绝 work 加入

\__queue_work 函数首先校验 workqueue 的 flag 参数是否设置了 \__WQ_DRAINING bit ， 设置 __WQ_DRAINING bit 表示当前 workqueue 正在进行 draining 操作，这多半是 workqueue 正在销毁，此时 workqueue 需要将已经加入的 work 都处理完，不再允许新的 work 加入。

但是有一种特例（通过 is_chained_work 判定），也就是正在清空的 work（隶属于该 workqueue ）又触发了一个 queue work 的操作（也就是所谓 chained work ），这时候该 work 允许加入。

相关代码如下：
```c
    if (unlikely(wq->flags & __WQ_DRAINING) && WARN_ON_ONCE(!is_chained_work(wq)))
            return; 
```

### 设置缺省 pool workqueue

对于非 unbound 的 workqueue ，选择当前 cpu 的 per cpu pool workqueue 为缺省 pool pwq ；对于 unbound 的 workqueue ， 根据 numa node id 来选择缺省 pool pwq  。
```c
	if (req_cpu == WORK_CPU_UNBOUND)
		cpu = wq_select_unbound_cpu(raw_smp_processor_id());

	if (!(wq->flags & WQ_UNBOUND))
		pwq = per_cpu_ptr(wq->cpu_pwqs, cpu);
	else
		pwq = unbound_pwq_by_node(wq, cpu_to_node(cpu));    /* cpu_to_node 可以从 cpu id 获取 node id */
```

### 选择 worker pool 和 pool workqueue

首先选择 worker pool 。

如果 work 未被任何 worker pool 执行过，或当前未被执行，则使用缺省 pwq 下的 worker pool

如果 work 正在被某个 worker pool 执行， 且该 worker thread 所属的 pwq 属于当期 wq ， 则继续选择当前 worker pool 和当前 pwq ； 否则使用缺省 pwq 下的 worker pool。

```c
    last_pool = get_work_pool(work);            /* 查看 work 上一次被哪个 worker pool 处理 */
    if (last_pool && last_pool != pwq->pool) {  /* last_pool 与 缺省 pwq 指向的 worker pool 不匹配，则使用 last_pool */
        struct worker *worker;

        spin_lock(&last_pool->lock);

        worker = find_worker_executing_work(last_pool, work);   /* 找到 work 正在被哪一个 worker thread 处理 */

        if (worker && worker->current_pwq->wq == wq) {  /* 如果 work 正在被当前 workqueue 的一个 pwq 下的 worker 处理，则继续选择这个 pwq  */
            pwq = worker->current_pwq;
        } else {    /* 选择缺省 pwq */
            /* meh... not running there, queue here */
            spin_unlock(&last_pool->lock);
            spin_lock(&pwq->pool->lock);
        }
    } else {    /* work 未被处理过，则选择缺省 pwq */
        spin_lock(&pwq->pool->lock);
    } 
```

### 为 work 选择挂入的队列

work 可选择的队列有两个：

- 延迟执行的队列（ pwq->delayed_works ），进入该队列的 work 保持 pending 状态。
- 将要处理的队列（ pwq->pool->worklist ），进入该队列的 work 进入 active 状态。

如果 pwq 下活动的 worker 未满（ pwq->nr_active < pwq->max_active ），则将 work 挂入将要处理的队列，否则挂入延迟执行的队列，代码如下：
```c
    pwq->nr_in_flight[pwq->work_color]++;
    work_flags = work_color_to_flags(pwq->work_color);

    if (likely(pwq->nr_active < pwq->max_active)) {
        pwq->nr_active++;
        worklist = &pwq->pool->worklist;
    } else {
        work_flags |= WORK_STRUCT_DELAYED;
        worklist = &pwq->delayed_works;
    }

    insert_work(pwq, work, worklist, work_flags);   /* 将 work 挂入 pwq 中的指定队列 */
```

### 唤醒 idle 的 worker 来处理该 work

在 insert_work 函数中，会判断 worker pool 中当前正在运行的 worker thread 数是否为 0，如果是，则需要唤醒一个 idle thread 来处理该 work。代码如下：
```c
if (__need_more_worker(pool))
        wake_up_worker(pool); 
```

## worker pool 如何创建 worker thread

### per cpu worker pool 创建 worker thread 的时机

workqueue 模块初始化的时候，就会初始化 per cpu workqueue 的 worker pool ， workqueue 中共享的普通和高优先级 worker pool 都会初始化
```c
static int __init init_workqueues(void)
{
    ...
    
    for_each_online_cpu(cpu) {
    struct worker_pool *pool;

    for_each_cpu_worker_pool(pool, cpu) {
        pool->flags &= ~POOL_DISASSOCIATED;
        BUG_ON(!create_worker(pool));   /* create_worker 创建 initial worker */
    }
    
    ...
}
```

### unbound worker pool 创建 worker thread 的时机

unbound thread pool 是全局共享的，因此，每当创建不同属性的 unbound workqueue 的时候，都需要创建 pool_workqueue 及其对应的 worker pool ，这时候就会调用 get_unbound_pool 函数在当前系统中现存的线程池中找是否有匹配的 worker pool ，如果没有就需要创建新的线程池。在创建新的线程池之后，会立刻调用 create_worker 创建一个 initial worker 。

### 创建 worker thread

```c
static struct worker *create_worker(struct worker_pool *pool)
{
    struct worker *worker = NULL;
    int id = -1;
    char id_buf[16];

    id = ida_simple_get(&pool->worker_ida, 0, 0, GFP_KERNEL);       /* 分配 ID */

    worker = alloc_worker(pool->node);      /* 分配 worker 的内存 */

    worker->pool = pool;
    worker->id = id;

    /* 设置 worker 的名字 */
    if (pool->cpu >= 0)    /* pool->cpu 是大于等于0 表示 pool 是 per cpu 的 */                
        snprintf(id_buf, sizeof(id_buf), "%d:%d%s", pool->cpu, id,  pool->attrs->nice < 0  ? "H" : "");
    else
        snprintf(id_buf, sizeof(id_buf), "u%d:%d", pool->id, id);

    worker->task = kthread_create_on_node(worker_thread, worker, pool->node,   "kworker/%s", id_buf);   /* 创建 worker thread ， 在 thread 中执行 worker_thread 函数 */

    set_user_nice(worker->task, pool->attrs->nice);     /* 创建 task 并设定 nice value */
    worker->task->flags |= PF_NO_SETAFFINITY; 
    worker_attach_to_pool(worker, pool); /* 建立 worker 和 worker pool 的关系 */

    spin_lock_irq(&pool->lock);
    worker->pool->nr_workers++;
    worker_enter_idle(worker);
    wake_up_process(worker->task);      /* 启动 worker thread */
    spin_unlock_irq(&pool->lock);

    return worker;
} 
```

## 处理 work

和传统 workqueue 一样，使用 worker_thread 函数来处理 work

### worker thread 设置 PF_WQ_WORKER flag

worker_thread 函数一开始会将 worker thread 的 flag 设置 PF_WQ_WORKER bit
```c
    worker->task->flags |= PF_WQ_WORKER; 
```

有了这样一个 flag ，调度器在调度当前进程 sleep 的时候可以检查这个准备 sleep 的进程是否是一个 worker thread，如果是的话，那么调度器不能鲁莽的调度到其他的进程，这时候，还需要找到该 worker thread 对应的线程池，唤醒一个 idle 的 worker 线程。通过 workqueue 模块和调度器模块的交互，当 work A 被阻塞后（处理该 work 的 worker 线程进入 sleep ），调度器会唤醒其他的 worker 线程来处理其他的 work B，work C…… 

### 管理 worker pool 中的线程

是否需要创建更多的worker线程，规则如下：

- 有事情做时，worker pool 中的 work list 不能是空的，如果是空的，表示无事可做，此时 sleep
- 比较忙时，worker pool 中的 work list 不能是空的，这时候需要看 worker pool 中正在运行的 worker thread 有多少（pool->nr_running），如果 nr_running = 0，表示 need more worker，如果 nr_running 非 0， 这时候也 sleep 好了。
```c
recheck:
    if (!need_more_worker(pool))    /* pool->worklist 为空，或没有正在运行的 thread ， 则让该 worker thread sleep*/
        goto sleep;

    if (unlikely(!may_start_working(pool)) && manage_workers(worker))
        goto recheck; 
```
### worker thread 中执行 work

```c
worker_clr_flags(worker, WORKER_PREP | WORKER_REBOUND);

do {
    struct work_struct *work =   list_first_entry(&pool->worklist,  struct work_struct, entry);

    if (likely(!(*work_data_bits(work) & WORK_STRUCT_LINKED))) {    /* 对于普通 work */
        process_one_work(worker, work);     /* 让 worker thread 执行一个 work */
        if (unlikely(!list_empty(&worker->scheduled)))              /* process_one_work 函数中 如果发现一个 work 正在被另一个 worker thread 执行，那么会将该 work 挂入 scheduled work list， 此时需要处理 scheduled work list 中的 work */
            process_scheduled_works(worker);
    } else {    /* 对于 linked work */
        move_linked_works(work, &worker->scheduled, NULL);  /* 挂入 scheduled work list */ 
        process_scheduled_works(worker);    /* process_scheduled_works 函数调用 process_one_work 来一个一个处理 scheduled work list 上的 work */
    }
} while (keep_working(pool));

worker_set_flags(worker, WORKER_PREP); 
```
- work 和 work 之间并不是独立的，也就是说， work A 和 work B 可能是 linked work ，这些 linked work 应该被一个 worker 来处理。WORK_STRUCT_LINKED 标记了 work 是属于 linked work ，如果是 linked work ， worker 并不直接处理，而是将其挂入 scheduled work list ，然后调用 process_scheduled_works 来处理。
- process_one_work 函数中，会检查 work 是否被其他 worker thread 执行，如果是，则将该 work 挂入 scheduled work list， 之后需要处理 scheduled work list 中的 work
