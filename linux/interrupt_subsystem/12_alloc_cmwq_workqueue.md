# CMWQ 创建 workqueue 代码分析

__alloc_workqueue_key 函数是 CMWQ 创建 workqueue 函数

## WQ_POWER_EFFICIENT 的处理

__alloc_workqueue_key 函数的一开始有如下的代码： 

```c
if ((flags & WQ_POWER_EFFICIENT) && wq_power_efficient)
        flags |= WQ_UNBOUND;  
```

```c
static bool wq_power_efficient = IS_ENABLED(CONFIG_WQ_POWER_EFFICIENT_DEFAULT);
```

使用 workqueue 需要在性能和能耗之间权衡，需要更好的性能，则最好让一个 idle cpu 上的 worker thread 来处理 work ，这样的话， cache 使用效率会比较高，性能会更好。如果需要地能耗，最好的策略是将 work 调度到 running 的 cpu 上的 worker thread ，而不是反复 idle， working ， idle again 。

对于 unbound 的 workqueue ， 调度器可以考虑 CPU 的 idle 状态（尽可能让 CPU 保持 idle），从而节省了能耗。所以，使用设置了   WQ_UNBOUND flag 的 wordqueue 有利于低能耗；如果 workqueue 没有设置 WQ_UNBOUND flag ，则说明该 workqueue 是 per cpu 的，这时候调度哪一个 CPU 上的 worker thread 来处理 work 就不是调度器可以控制的。

所以，如果 alloc_workqueue 时指定了 WQ_POWER_EFFICIENT flag ， 即使没有设置 WQ_UNBOUND flag， WQ_UNBOUND 也会强制加上。 这个功能需要内核配置了 CONFIG_WQ_POWER_EFFICIENT_DEFAULT ， 并且设置 wq_power_efficient = true 。
```c
#ifdef CONFIG_WQ_POWER_EFFICIENT_DEFAULT
static bool wq_power_efficient = true;
#else
static bool wq_power_efficient;
#endif

module_param_named(power_efficient, wq_power_efficient, bool, 0444); 
```

## 分配 workqueue 的内存

### CMWQ 中的 workqueue_struct 结构体

CMWQ 中的 workqueue_struct 结构体大致定义如下：
```c
struct workqueue_struct {
    struct list_head    pwqs; 
    struct list_head    list;
    
    ...
    struct workqueue_attrs	*unbound_attrs;	    /* PW: only for unbound wqs */
    ...
    struct pool_workqueue __percpu *cpu_pwqs;       /* 指向 per cpu 的 pool workqueue  */
    struct pool_workqueue __rcu *numa_pwq_tbl[];    /* 指向 per node 的 pool workqueue */
}; 
```
在这个版本的 workqueue_struct 中共享一组 thread pool ， 这些 thread pool 包括 global 类型的和 per thread pool 类型的。 那些 per thread pool 类型的 thread pool 集合起来，就是 pool_workqueue 的定义。

挂入 workqueue 的 work 终究需要 worker pool 中的某个 worker thread 来处理，也就是说，workqueue 要和系统中那些共享的 worker pool 进行连接，这是通过 pool_workqueue （该数据结构会包含一个指向 worker pool 的指针）的数据结构来管理的。和 workqueue 相关的 pool_workqueue 被挂入一个链表，链表头就是 workqueue_struct 中的 pwqs 成员。

和旧的 workqueue 机制一样，系统维护了一个所有 workqueue 的 list ， list head 定义如下： 
```c
static LIST_HEAD(workqueues); 
```

workqueue_struct 中的 list 成员就是挂入这个链表的节点。

cpu_pwqs 指针指向一组 per cpu 的 pool_workqueue 数据结构，用来维护 workqueue 和 per cpu thread pool 之间的关系，一般情况下默认指向普通的 worker pool ，如果 workqueue 的 flag 成员设置了 WQ_HIGHPRI ， 就指向高优先级的 worker pool 。

### workqueue attribute

挂入 workqueue 的 work 终究是需要 worker thread 来处理，针对 worker thread 有下面几个考量点（我们称之 attribute ）：

- 该 worker thread 的优先级
- 该 worker thread 运行在哪一个 CPU 上 
- 如果 worker thread 可以运行在多个 CPU 上，且这些 CPU 属于不同的 NUMA node ，那么是否在所有的 NUMA node 中都可以获取良好的性能。

对于 per cpu 的 workqueue ， 2 和 3 不是问题，哪个 cpu 上 queue 的 work 就在哪个 cpu 上执行，由于只能在一个确定的 cpu 上执行，因此起 NUMA 的 node 也是确定的（一个 CPU 不可能属于两个 NUMA node ）。至于问题 1 ， per cpu 的 workqueue 使用 WQ_HIGHPRI 来标记为高优先级。所以， per cpu 的 workqueue 不需要单独定义一个 workqueue attribute ，这也是为何在 workqueue_struct 中只有 unbound_attrs 这个成员来记录 unbound workqueue 的属性。 

workqueue_attrs 定义如下：
```c
struct workqueue_attrs {
    int            nice;            /* nice level */
    cpumask_var_t        cpumask;   /* allowed CPUs */
    bool            no_numa;        /* disable NUMA affinity */
}; 
```
- nice 越低则优先级越高
- cpumask 是该 workqueue 挂入的 work 允许在哪些 cpu 上运行
- no_numa 是一个和 NUMA affinity 相关的设定

### unbound workqueue 和 NUMA 之间的关系

NUMA （ Non Uniform Memory Access 统一内存访问 ）系统，不同的一个或者一组 cpu 看到的 memory 是不一样的，我们假设 node 0 中有 CPU A 和 B ， node 1 中有 CPU C 和 D ，如果运行在 CPU A 上内核线程现在要迁移到 CPU C 上的时候，悲剧发生了：该线程在 A CPU 创建并运行的时候，分配的内存是 node 0中的 memory，这些 memory 是 local 的访问速度很快，当迁移到 CPU C 上的时候，原来 local memory 变成 remote，性能大大降低。因此，unbound workqueue 需要引入 NUMA 的考量点。 

暂时不考虑 NUMA ， 先希考这个问题： 一个确定属性的 unbound workqueue 需要几个 worker pool ？看起来一个就够了，毕竟 workqueue 的属性已经确定了，一个 worker pool 创建相同属性的 worker thread 就行了。但是我们来看一个例子：假设 workqueue 的 work 是可以在 node 0 中的 CPU A 和 B ，以及 node 1 中 CPU C 和 D 上处理，如果只有一个 thread pool ，那么就会存在 worker thread 在不同 node 之间的迁移问题。为了解决这个问题，实际上 unbound workqueue 实际上是创建了 per node 的 pool_workqueue （ thread pool ）

是否使用 per node 的 pool workqueue 用户是可以通过 workqueue attribute 中的 no_numa 成员设置，也可以通过 workqueue.disable_numa 这个参数 disable 所有 workqueue 的 numa affinity 的支持。
```c
static bool wq_disable_numa;
module_param_named(disable_numa, wq_disable_numa, bool, 0444)；
```

### unbound workqueue 的内存分配

__alloc_workqueue_key 函数中， workqueue 内存分配代码如下：
```c
if (flags & WQ_UNBOUND)
    tbl_size = nr_node_ids * sizeof(wq->numa_pwq_tbl[0]); /* only for unbound workqueue */

wq = kzalloc(sizeof(*wq) + tbl_size, GFP_KERNEL);

if (flags & WQ_UNBOUND) {
        wq->unbound_attrs = alloc_workqueue_attrs(GFP_KERNEL); /* only for unbound workqueue */
} 
```
- 对于 per cpu 的 workqueue ， 直接分配内存
- 对于 unbound 的 workqueue ， 则需要 numa_pwq_tbl 指针数组首元素和 unbound_attrs 分配内存

### 初始化 workqueue 的成员

__alloc_workqueue_key 函数中， 初始化 workqueue 成员代码如下：
```c
    va_start(args, lock_name);
    vsnprintf(wq->name, sizeof(wq->name), fmt, args);       /* set workqueue name */
    va_end(args);

    max_active = max_active ?: WQ_DFL_ACTIVE;               /* 用户没有设定 max active （或者说 max active 等于0），系统使用默认值 WQ_MAX_ACTIVE/2*/
    /* 对于 per cpu workqueue ， max_active 取值在 1 到 WQ_MAX_ACTIVE ； 对于 unbound workqueue , max_active 取值最小值为 1 ， 最大值在 cpu num * 4 到 WQ_MAX_ACTIVE 之间 */
    max_active = wq_clamp_max_active(max_active, flags, wq->name);
    wq->flags = flags;
    wq->saved_max_active = max_active;
    mutex_init(&wq->mutex);
    atomic_set(&wq->nr_pwqs_to_flush, 0);
    INIT_LIST_HEAD(&wq->pwqs);
    INIT_LIST_HEAD(&wq->flusher_queue);
    INIT_LIST_HEAD(&wq->flusher_overflow);
    INIT_LIST_HEAD(&wq->maydays);

    lockdep_init_map(&wq->lockdep_map, lock_name, key, 0);
    INIT_LIST_HEAD(&wq->list); 
```

### 分配 pool workqueue 的内存并建立 workqueue 和 pool workqueue 的关系

__alloc_workqueue_key 函数中， 调用 alloc_and_link_pwqs 函数为 pool workqueue 分配内存，并建立 workqueue 和 pool workqueue 之间的关系。

```c
static int alloc_and_link_pwqs(struct workqueue_struct *wq)
{
    bool highpri = wq->flags & WQ_HIGHPRI;  /* normal or high priority？ */
    int cpu, ret;

    if (!(wq->flags & WQ_UNBOUND)) {        /* 设置 per cpu workqueue */
        wq->cpu_pwqs = alloc_percpu(struct pool_workqueue);     /* 每一个 cpu 分配一个 pool_workqueue 的 memory */

        for_each_possible_cpu(cpu) {        /* 设置每个 cpu 的 pool workqueue */
            struct pool_workqueue *pwq =    per_cpu_ptr(wq->cpu_pwqs, cpu);     /* 获取 per cpu pool workqueues 地址 */
            struct worker_pool *cpu_pools = per_cpu(cpu_worker_pools, cpu); /* 获取 per cpu 两个 worker pools 地址 */

            init_pwq(pwq, wq, &cpu_pools[highpri]);     /* 初始化 pwq ， 设置其对应的 workqueue 和 worker pool （ 指向普通或高优先级 worker pool ） */
            link_pwq(pwq);                  /* pool_workqueue 挂入它所属的 workqueue 的链表 */
        }
        return 0;
    } else if (wq->flags & __WQ_ORDERED) {  /* ordered unbound workqueue 的处理 */
        ret = apply_workqueue_attrs(wq, ordered_wq_attrs[highpri]);
        return ret;
    } else {                                /* unbound workqueue 的处理 */
        return apply_workqueue_attrs(wq, unbound_std_wq_attrs[highpri]);    /* 分配 pool workqueue 并建立 workqueue 和 pool workqueue 的关系 */
    }
}

static void init_pwq(struct pool_workqueue *pwq, struct workqueue_struct *wq,
		     struct worker_pool *pool)
{
	memset(pwq, 0, sizeof(*pwq));

	pwq->pool = pool;
	pwq->wq = wq;
    
    ...
} 
```

#### 设置 per cpu workqueue

per cpu 的 workqueue 的 pool workqueue 对应的 worker pool 也是 per cpu 的，每个 cpu 有两种 worker pool （ normal 和 high priority ），因此只需要根据 workqueue 是否设置了 WQ_HIGHPRI ， 而将 pool workqueue 指向普通或高优先级的 worker pool 即可。

cpu_worker_pools 是静态定义的 per cpu 的 worker pool 数组，长度为 2， 第一个元素是普通的 worker pool ， 第二个元素是高优先级的 worker pool ， 定义如下：
```c
static DEFINE_PER_CPU_SHARED_ALIGNED(struct worker_pool [NR_STD_WORKER_POOLS], cpu_worker_pools);
```

#### 设置 unbound workqueue

unbound workqueue 有两种：

- normal type unbound workqueue
- ordered type unbound workqueue ，这种 workqueue 上的 work 是严格按照顺序执行的，不存在并发问题。 ordered unbound workqueue 的行为类似过去的 single thread workqueue 。

无论那种类型的 unbound workqueue 都使用 apply_workqueue_attrs 函数为 pool workqueue 分配内存并建立 workqueue 和 pool workqueue 的关系

##### 健康检查

只有 unbound 类型的 workqueue 才有 attribute ，才可以 apply attributes 。对于 ordered 类型的 unbound workqueue ，属于它的 pool workqueue （ worker pool ）只能有一个，否则无法限制 work 是按照顺序执行。 
```c
    if (WARN_ON(!(wq->flags & WQ_UNBOUND)))
        return -EINVAL;

    if (WARN_ON((wq->flags & __WQ_ORDERED) && !list_empty(&wq->pwqs)))
        return -EINVAL; 
```

##### 分配内存并初始化 workqueue_attrs

在 apply_wqattrs_prepare 函数中为 workqueue_attrs 和 pool workqueue 分配内存
```c
static struct apply_wqattrs_ctx * apply_wqattrs_prepare(struct workqueue_struct *wq, 		      const struct workqueue_attrs *attrs)
{
    struct apply_wqattrs_ctx *ctx;
    struct workqueue_attrs *new_attrs, *tmp_attrs;
    
    ...
    
    ctx = kzalloc(sizeof(*ctx) + nr_node_ids * sizeof(ctx->pwq_tbl[0]),GFP_KERNEL); /* 为 apply_wqattrs_ctx 分配内存 */
    
    new_attrs = alloc_workqueue_attrs(GFP_KERNEL);
    tmp_attrs = alloc_workqueue_attrs(GFP_KERNEL);  /* 用于计算 workqueue attribute 的中间变量 */
    
    ...
    
    copy_workqueue_attrs(new_attrs, attrs);
    cpumask_and(new_attrs->cpumask, new_attrs->cpumask, cpu_possible_mask); /* 设置 workqueue_attrs */
    
    ...
    
    copy_workqueue_attrs(tmp_attrs, new_attrs);
    
    ...
}
```

apply_wqattrs_ctx 结构体用于预先保存匹配的 workqueue_attrs 和 unbound workqueue 各个 node 的 pool workqueue 的指针
```c
struct apply_wqattrs_ctx {
	struct workqueue_struct	*wq;		/* target workqueue */
	struct workqueue_attrs	*attrs;		/* attrs to apply */
	struct list_head	list;		/* queued for batching commit */
	struct pool_workqueue	*dfl_pwq;   /* 保存 unbound workqueue 缺省的 pool workqueue 的指针 */
	struct pool_workqueue	*pwq_tbl[]; /* 保存 unbound workqueue 各个 node 的 pool workqueue 的指针 */
};
```

##### 分配 pool workqueue 内存
apply_wqattrs_prepare 函数中使用 alloc_unbound_pwq 函数分配 pool workqueue
```c
 
    ...
    
	ctx->dfl_pwq = alloc_unbound_pwq(wq, new_attrs);
	if (!ctx->dfl_pwq)
		goto out_free;

	for_each_node(node) {
		if (wq_calc_node_cpumask(new_attrs, node, -1, tmp_attrs->cpumask)) {
			ctx->pwq_tbl[node] = alloc_unbound_pwq(wq, tmp_attrs);
			if (!ctx->pwq_tbl[node])
				goto out_free;
		} else {
			ctx->dfl_pwq->refcnt++;
			ctx->pwq_tbl[node] = ctx->dfl_pwq;
		}
	}
    
    ...

```

alloc_unbound_pwq 函数中最核心问题就是获得和 workqueue_attrs 匹配的 worker pool , 因为 unbound workqueue 的 pool workqueue 和 worker pool 之间的关系较复杂，如果属性相同，多个 unbound workqueue 的 pool workqueue 可能对应一个 worker pool 。

通过 get_unbound_pool 函数来解决这个问题，在创建 unbound workqueue 的时候，pool workqueue 对应的 worker pool 需要在这个哈希表中搜索，如果有相同属性的 worker pool 的话，那么就不需要创建新的 worker pool ，如果没有相同属性的 worker pool，那么需要创建一个并挂入哈希表，代码如下：
```c
static struct worker_pool *get_unbound_pool(const struct workqueue_attrs *attrs)
{
    ... 
       
    hash_for_each_possible(unbound_pool_hash, pool, hash_node, hash) {
        if (wqattrs_equal(pool->attrs, attrs)) { /* 检查属性是否相同 */
            pool->refcnt++;
            return pool;    /* 在哈希表 unbound_pool_hash 找到适合的 unbound worker pool */
        }
    }
    
    ...  
    
    pool = kzalloc_node(sizeof(*pool), GFP_KERNEL, target_node);    /* 如果有需要，则创建新的 worker pool */
    
    ...
    
    hash_add(unbound_pool_hash, &pool->hash_node, hash);
    
    ... 
}
```

系统使用哈希表来保存所有的 unbound worker pool，定义如下： 
```c
static DEFINE_HASHTABLE(unbound_pool_hash, UNBOUND_POOL_HASH_ORDER); 
```

##### 为各个 node 分配 pool workqueue ， 并初始化
回到 apply_wqattrs_prepare 函数，接着为各个 node 分配 pool workqueue ， 并初始化
```c
    ...
    
    ctx->dfl_pwq = alloc_unbound_pwq(wq, new_attrs);    /* 分配 default pool workqueue */
    
    ...
    
	for_each_node(node) {       /* 遍历 node */
		if (wq_calc_node_cpumask(new_attrs, node, -1, tmp_attrs->cpumask)) {    /* 是否使用 default pool workqueue */
			ctx->pwq_tbl[node] = alloc_unbound_pwq(wq, tmp_attrs);  /* 为该 node 分配 pool workqueue */
			if (!ctx->pwq_tbl[node])
				goto out_free;
		} else {
			ctx->dfl_pwq->refcnt++;
			ctx->pwq_tbl[node] = ctx->dfl_pwq;  /* 该 node 使用 default pool workqueue */
		}
	}
    
    ...
```
- wq_calc_node_cpumask 函数会根据该 node 的 cpu 情况以及 workqueue attribute 中的 cpumask 成员来更新 tmp_attrs->cpumask，因此，在 ctx->pwq_tbl[node] = alloc_unbound_pwq(wq, tmp_attrs); 这行代码中，为该 node 分配 pool workqueue 对应的 worker pool 的时候，去掉了本 node 中不存在的cpu。例如 node 0 中有 CPU A 和 B ， workqueue 的 attribute 规定 work 只能在 CPU A 和 C 上运行，那么创建 node 0 上的 pool workqueue 以及对应的 worker pool 的时候，需要删除 CPU C ，也就是说， node 0 上的 worker pool 的属性中的 cpumask 仅仅支持 CPU A 。


##### 建立 unbound workqueue 和 pool workqueue 、 worker pool

回到 apply_workqueue_attrs 函数， 现在所有的 node 的 pool workqueue 及其 worker pool 已经就绪，就需要安装到 unbound workqueue 中。
```c
/* set attrs and install prepared pwqs, @ctx points to old pwqs on return */
static void apply_wqattrs_commit(struct apply_wqattrs_ctx *ctx)
{
	int node;

	/* all pwqs have been created successfully, let's install'em */
	mutex_lock(&ctx->wq->mutex);

	copy_workqueue_attrs(ctx->wq->unbound_attrs, ctx->attrs);

	/* save the previous pwq and install the new one */
	for_each_node(node)
		ctx->pwq_tbl[node] = numa_pwq_tbl_install(ctx->wq, node, ctx->pwq_tbl[node]);

	/* @dfl_pwq might not have been used, ensure it's linked */
	link_pwq(ctx->dfl_pwq);
	swap(ctx->wq->dfl_pwq, ctx->dfl_pwq);

	mutex_unlock(&ctx->wq->mutex);
}
```




















