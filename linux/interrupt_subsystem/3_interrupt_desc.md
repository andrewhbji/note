# IRQ number 和 中断描述符

## 基本概念

### 通用中断的代码处理示意图

![通用中断的代码处理示意图](./img/int_handler_workflow.gif)

在 linux kernel 中，对于每一个外设的 IRQ 都用 struct irq_desc 来描述，我们称之中断描述符（struct irq_desc）。linux kernel 中会有一个数据结构保存了关于所有IRQ的中断描述符信息，我们称之中断描述符 DB（上图中红色框图内）。

当发生中断后，首先获取触发中断的 HW interupt ID，然后通过 irq domain 翻译成 IRQ number，再通过 IRQ number 就可以获取对应的中断描述符，最后调用中断描述符中的 highlevel irq-events handler 进行中断处理。

highlevel irq-events handler 进行两个操作：

- 调用中断描述符的底层 irq chip driver 进行 mask，ack 等 callback 函数，进行 interrupt flow control。
- 调用该中断描述符上的 action list 中的 specific handler（有别于中断 handler 和 high level handler）。这个步骤不一定会执行，这是和中断描述符的当前状态相关，实际上，interrupt flow control 是软件（设定一些标志位，软件根据标志位进行处理）和硬件（mask 或者 unmask interrupt controller 等）一起控制完成。

### 中断的打开和关闭

通用中断处理过程中的开关中断有两种：

- 开关 local CPU 的中断。对于 UP，关闭 CPU 中断就关闭了一切，永远不会被抢占。对于 SMP，只能关闭 local CPU（代码运行的那个CPU）
- 控制 interrupt controller，关闭某个 IRQ number 对应的中断。更准确的术语是 mask 或者 unmask 一个 IRQ。

对于一种情况：

- 当进入 high level handler 的时候，CPU 的中断是关闭的（硬件在进入 IRQ processor mode 的时候设定的）

- 从 high level handler 转入 specific handler，也要确保其中断处理过程中是关闭 CPU 的中断

### IRQ number

通用中断处理模块用一个中断描述符来描述 HW interupt ID，并为每一个外设的 interrupt request line 分配一个中断号（irq number），所以 irq domain 中有多少个 irq request line 就有多少个中断描述符（struct irq_desc），NR_IRQS 定义了该硬件平台 irq request line

从 CPU 的角度看，通用中断处理模块不关心外部 interrupt controller 的组织细节，只关注一个指定外设的发生中断时，调用相应的外设中断的 handler。简而言之，通用中断处理模块可以用一个线性的 table 来管理一个个的外部中断，这个表的每个元素就是一个中断描述符，在kernel中定义如下：
```c
struct irq_desc irq_desc[NR_IRQS] __cacheline_aligned_in_smp = {
    [0 ... NR_IRQS-1] = {
        .handle_irq    = handle_bad_irq,
        .depth        = 1,
        .lock        = __RAW_SPIN_LOCK_UNLOCKED(irq_desc->lock),
    }
}; 
```

使用静态表格，以 irq number 为 index，需要每个中断描述符都紧密排在一起。但是有些系统可能会定义一个很大的NR_IRQS，却只是想用其中的若干个，这样， 这个静态定义的表格不是每个 entry 都是有效的，会存在内存浪费。这种情况下，就需要改用 radix tree 来保存中断描述符（以 HW interrupt 作为索引）。使用 radix tree 需要打开 CONFIG_SPARSA_IRQ 选项

引入 irq domain 后， IRQ number 不再和硬件有任何关系

## 中断描述符数据结构

### 底层 irq chip 相关的数据结构

中断描述符中应该会包括底层 irq chip 相关的数据结构，linux kernel中把这些数据组织在一起，形成 struct irq_data
```c
struct irq_data {
    u32            mask;
    unsigned int        irq;            /* IRQ number */
    unsigned long        hwirq;         /* HW interrupt ID */
    unsigned int        node;           /* NUMA node index */
    unsigned int        state_use_accessors; /* 底层状态，参考 IRQD_xxxx */
    struct irq_chip        *chip;       /* 该中断描述符对应的 irq chip 数据结构 */
    struct irq_domain    *domain;       /* 该中断描述符对应的 irq domain 数据结构 */
    void            *handler_data;      /* 和外设 specific handler 相关的私有数据 */
    void            *chip_data;         /* 和中断控制器相关的私有数据 */
    struct msi_desc        *msi_desc;
    cpumask_var_t        affinity;      /* 和irq affinity相关 */
}; 
```

中断有两种形态，一种就是直接通过 signal 传递，通过信号线上的电平或者边缘（电流强度的上升沿/下降沿）触发。另外一种是基于消息的，被称为 MSI (Message Signaled Interrupts)。msi_desc 是 MSI 类型的中断相关。

node 成员用来保存中断描述符的内存位于哪一个 memory node 上。 对于支持 NUMA（Non Uniform Memory Access Architecture） 的系统，其内存空间并不是均一的，而是被划分成不同的 node，对于不同的 memory node ，CPU 其访问速度是不一样的。如果一个 IRQ 大部分（或者固定）由某一个 CPU 处理，那么在动态分配中断描述符的时候，应该考虑将内存分配在该 CPU 访问速度比较快的 memory node 上。

### irq chip 结构

Interrupt controller 描述符（struct irq_chip）包括了若干和具体 Interrupt controller 相关的 callback 函数：

- name 	该中断控制器的名字，用于/proc/interrupts中的显示
- irq_startup 	start up 指定的 irq domain 上的 HW interrupt ID。如果不设定的话，default 会被设定为 enable 函数
- irq_shutdown 	shutdown 指定的 irq domain 上的 HW interrupt ID 。如果不设定的话，default 会被设定为 disable 函数
- irq_enable 	enable 指定的 irq domain 上的 HW interrupt ID 。如果不设定的话， default 会被设定为unmask函数
- irq_disable 	disable 指定的 irq domain 上的 HW interrupt ID。
- irq_ack 	和具体的硬件相关，有些中断控制器必须在Ack之后（清除pending的状态）才能接受到新的中断。
- irq_mask 	mask 指定的 irq domain 上的 HW interrupt ID
- irq_mask_ack 	mask 并 ack 指定的 irq domain 上的 HW interrupt ID。
- irq_unmask 	unmask 指定的 irq domain 上的 HW interrupt ID
- irq_eoi 	有些 interrupt controler（例如 GIC）提供了这样的寄存器接口，让 CPU 可以通知 interrupt controller，它已经处理完一个中断
- irq_set_affinity 	在 SMP 的情况下，可以通过该 callback 函数设定 CPU affinity
- irq_retrigger 	重新触发一次中断，一般用在中断丢失的场景下。如果硬件不支持retrigger，可以使用软件的方法。
- irq_set_type 	设定指定的 irq domain 上的 HW interrupt ID 的触发方式是电平触发还是边缘触发
- irq_set_wake 	和电源管理相关，用来 enable/disable 指定的 interrupt source 作为唤醒的条件。
- irq_bus_lock 	有些 interrupt controller 是连接到慢速总线上（例如一个 i2c 接口的 IO expander 芯片），在访问这些芯片的时候需要 lock 住那个慢速 bus（只能有一个 client 在使用 I2C bus）
- irq_bus_sync_unlock 	unlock 慢速总线
- irq_suspend
- irq_resume
- irq_pm_shutdown 	电源管理相关的 callback 函数
- irq_calc_mask 	TODO
- irq_print_chip 	/proc/interrupts 中的信息显示

### 中断描述符

在 linux kernel 中，使用 struct irq_desc 来描述中断描述符
```c
struct irq_desc {
    struct irq_data        irq_data;
    unsigned int __percpu    *kstat_irqs;           /* IRQ 的统计信息 */
    irq_flow_handler_t    handle_irq;
    struct irqaction    *action; 
    unsigned int        status_use_accessors;       /* 中断描述符的状态，参考 IRQ_xxxx */
    unsigned int        core_internal_state__do_not_mess_with_it;
    unsigned int        depth;
    unsigned int        wake_depth;
    unsigned int        irq_count;                  /* 记录发生的中断的次数，每 100,000 则回滚 */
    unsigned long        last_unhandled;            /* 上一次没有处理的IRQ的时间点 */
    unsigned int        irqs_unhandled;             /* 没有处理的次数 */
    atomic_t		threads_handled;
	int			threads_handled_last;
    raw_spinlock_t        lock;                     /* 保护该中断描述符的 spin lock */
    struct cpumask        *percpu_enabled;
#ifdef CONFIG_SMP
    const struct cpumask    *affinity_hint;         /* 和 irq affinity 相关 */
    struct irq_affinity_notify *affinity_notify;
#ifdef CONFIG_GENERIC_PENDING_IRQ
    cpumask_var_t        pending_mask;
#endif
#endif
    unsigned long        threads_oneshot; /* threads_oneshot、threads_active 和 wait_for_threads 是和 IRQ thread 相关 */
    atomic_t        threads_active;
    wait_queue_head_t       wait_for_threads;
#ifdef CONFIG_PROC_FS
    struct proc_dir_entry    *dir;                  /* 该 IRQ 对应的 proc 接口 */
#endif
    int            parent_irq;
    struct module        *owner;
    const char        *name;
} ____cacheline_internodealigned_in_smp 
```
- handle_irq 就是 high level irq-events handler，与 specific handler （处理具体事件的 handler，如按键中断、磁盘中断）相比， highlevel 则是对处理各种中断交互过程的一个抽象。high level irq-events handler可以分成：

    - 处理电平触发类型的中断handler（ handle_level_irq ）
    - 处理边缘触发类型的中断handler（ handle_edge_irq ） 
    - 处理简单类型的中断handler（ handle_simple_irq ）
    - 处理EOI类型的中断handler（ handle_fasteoi_irq ） 
    
- action 指向一个 struct irqaction 的链表。如果一个 interrupt request line 允许共享，那么该链表中的成员可以是多个，否则，该链表只有一个节点。
- core_internal_state__do_not_mess_with_it 可以被简化成 istate，表示 internal state，最好不要直接修改它。
- depth 是描述 对 IRQ 进行 enable 和 disable 操作的嵌套深度，可以通过 enable/disable 一个指定的 IRQ 来控制内核的并发。
- wake_depth 和电源管理中的 wakeup source 相关。通过 irq_set_irq_wake 接口可以 enable /disable 一个 IRQ 中断是否可以把系统从 suspend 状态唤醒。同样的，对一个 IRQ 进行 wakeup source 的 enable/disable 的操作可以嵌套（需要成对使用），wake_depth 是描述嵌套深度的信息。
- irq_count、last_unhandled 和 irqs_unhandled 用于处理 broken IRQ 的处理。所谓 broken IRQ 就是由于种种原因（例如错误 firmware），IRQ handler 没有定向到指定的 IRQ 上，当一个 IRQ 没有被处理的时候，kernel 可以为这个没有被处理的 handler 启动 scan 过程，让系统中所有的 handler 来认领该 IRQ。
- percpu_enabled 是一个描述该 IRQ 在各个 CPU 上是否 enable。一个中断描述符可能会有两种情况，一种是该 IRQ 是 global，一旦 disable 了该 irq，那么对于所有的CPU而言都是 disable 的。还有一种情况，就是该 IRQ 是 Per-CPU（从属于CPU） 的，也就是说，在某个 CPU 上 disable 了该 irq 只是 disable 了本 CPU 的 IRQ 而已，其他的 CPU 仍然是 enable 的。

## 初始化中断描述符相关的结构

中断描述符在 start kernel 的时候初始化： start_kernel()->early_irq_init

### 静态定义的中断描述符初始化
```c
struct irq_desc irq_desc[NR_IRQS] __cacheline_aligned_in_smp = {
	[0 ... NR_IRQS-1] = {
		.handle_irq	= handle_bad_irq,
		.depth		= 1,
		.lock		= __RAW_SPIN_LOCK_UNLOCKED(irq_desc->lock),
	}
};

int __init early_irq_init(void)
{
    int count, i, node = first_online_node;
    struct irq_desc *desc;

    init_irq_default_affinity();

    desc = irq_desc;
    count = ARRAY_SIZE(irq_desc);

    for (i = 0; i < count; i++) {                           /* 遍历整个 lookup table，对每一个 entry 进行初始化 */
        desc[i].kstat_irqs = alloc_percpu(unsigned int);    /* 分配 per cpu 的 irq 统计信息需要的内存 */
        alloc_masks(&desc[i], GFP_KERNEL, node);            /* 分配中断描述符中需要的 cpu mask 内存 */
        raw_spin_lock_init(&desc[i].lock);                  /* 初始化 spin lock */
        lockdep_set_class(&desc[i].lock, &irq_desc_lock_class);
        desc_set_defaults(i, &desc[i], node, NULL);         /* 设定 default 值 */
    }
    return arch_early_irq_init();                           /* 调用 arch 相关的初始化函数 */
} 
```

### 使用 Radix tree 的中断描述符初始化 
```c
int __init early_irq_init(void)
{……

    initcnt = arch_probe_nr_irqs();         /* 体系结构相关的代码来决定预先分配的中断描述符的个数 */

    if (initcnt > nr_irqs)                  /* initcnt 是需要在初始化的时候预分配的 IRQ 的个数 */
        nr_irqs = initcnt;                  /* nr_irqs 是当前系统中 IRQ number 的最大值 */

    for (i = 0; i < initcnt; i++) {         /* 预先分配 initcnt 个中断描述符 */
        desc = alloc_desc(i, node, NULL);   /* 分配中断描述符 */
        set_bit(i, allocated_irqs);         /* 设定已经 alloc 的 flag */
        irq_insert_desc(i, desc);           /* 插入 radix tree */
    }
  
    return arch_early_irq_init();
} 
```

arch_probe_nr_irqs 函数会预先分配一定数量的 irq 描述符个数
```c
int __init arch_probe_nr_irqs(void)
{
    nr_irqs = machine_desc->nr_irqs ? machine_desc->nr_irqs : NR_IRQS;
    return nr_irqs;
} 
```

### 分配和释放中断描述符

使用 Radix tree 来保存中断描述符 DB 的 linux kernel，其中断描述符是使用 alloc_desc 函数动态分配的

也可以使用 irq_alloc_descs 和 irq_free_descs 来分配和释放中断描述符

最大可以分配的 ID 是 IRQ_BITMAP_BITS，定义如下： 

```c
#ifdef CONFIG_SPARSE_IRQ
# define IRQ_BITMAP_BITS    (NR_IRQS + 8196)    /* 对于Radix tree，除了预分配的，还可以动态 */
#else                                                                             分配8196个中断描述符
# define IRQ_BITMAP_BITS    NR_IRQS             /* 对于静态定义的，IRQ最大值就是NR_IRQS */
#endif 
```

## 和中断控制器相关的中断描述符的接口 

和中断控制器相关的中断描述符的操作接口主要分为两类：irq_desc_get_xxx 和 irq_set_xxx，一些 locked 版本的 set 接口，被定义为 __irq_set_xxx，这些 API 的调用者需要持有保护 irq desc 的 spinlock

### 接口调用的时机

kernel 提供了若干的接口 API 可以让内核其他模块可以操作指定 IRQ number 的描述符结构。中断描述符中有很多的成员是和底层的中断控制器相关，例如：

- 该中断描述符对应的irq chip
- 该中断描述符对应的irq trigger type
- high level handler 

在引入了device tree、动态分配 IRQ number 以及 irq domain 这些概念之后，这些接口的调用时机移到各个中断控制器的初始化以及各个具体硬件驱动初始化过程中，具体如下：

- 各个中断控制器定义好自己的struct irq_domain_ops callback函数，主要是map和translate函数。
- 在各个具体的硬件驱动初始化过程中，通过 device tree 可以知道自己的中断信息（连接到哪一个 interrupt controler、使用该 interrupt controller 的哪个 HW interrupt ID，trigger type 的类型），调用对应的 irq domain 的 translate 进行翻译、解析。之后可以动态申请一个 IRQ number 并和该硬件外设的 HW interrupt ID 进行映射，调用 irq domain 对应的 map 函数。在 map 函数中，可以调用本节定义的接口进行中断描述符底层 interrupt controller 相关信息的设定。

### irq_set_chip

这个接口函数用来设定中断描述符中desc->irq_data.chip成员，具体代码如下：
```c
int irq_set_chip(unsigned int irq, struct irq_chip *chip)
{
    unsigned long flags;
    struct irq_desc *desc = irq_get_desc_lock(irq, &flags, 0);  /* 获取 irq number 对应的中断描述符 */

    desc->irq_data.chip = chip;
    irq_put_desc_unlock(desc, flags);                           /* 恢复中断 flag */

    irq_reserve_irq(irq);                                       /* 函数标识该 IRQ number 已经分配 */
    return 0;
} 
```
- irq_get_desc_lock 函数以关闭中断＋ spin lock 来的方式保护中断描述符，flag 中就是保存的关闭中断之前的状态 flag ,操作完成后恢复中断 flag
- 中断描述符有用静态表格定义的，也有用 radix tree 维护的，对于静态表格定义的中断描述符，没有 alloc 的概念，irq_reserve_irq 函数能标识该 IRQ number 已经分配；但是对于使用 radix tree 维护的 中断描述符，因为已经使用 alloc_desc 、irq_alloc_desc、irq_alloc_descs 来分配，所以 irq_reserve_irq 函数对于 radix tree 而言是个重复的操作

### irq_set_irq_type

这个函数是用来设定该 irq number 的 trigger type
```c
int irq_set_irq_type(unsigned int irq, unsigned int type)
{
    unsigned long flags;
    struct irq_desc *desc = irq_get_desc_buslock(irq, &flags, IRQ_GET_DESC_CHECK_GLOBAL);
    int ret = 0;

    type &= IRQ_TYPE_SENSE_MASK;
    ret = __irq_set_trigger(desc, irq, type);
    irq_put_desc_busunlock(desc, flags);
    return ret;
} 
```
- 这里使用 irq_get_desc_buslock 代替 irq_get_desc_lock，是因为 irq_set_desc_buslock 需要通过 desc->irq_data.chip->irq_set_type 函数设置 interrupt controller，对于 SOC 内部的 interrupt controller 可以通过中断控制器的寄存器（memory map 到 CPU 的地址空间）实现快速访问，因此关闭中断＋spin lock没有问题，但是如果该 interrupt controller 是一个 I2C 接口的 IO 扩展芯片（也提供中断功能），直接对其进行 spin lock 操作会浪费 CPU 时间，这时候最好选择 bus lock 的方式 
- 对于irq_set_irq_type 函数，它是 for 1-N mode 的 interrupt source 使用的，所以要使用 IRQ_GET_DESC_CHECK_GLOBAL 宏。如果底层通过 IRQ_GET_DESC_CHECK_PERCPU 宏设定该 interrupt 是 per CPU 的，那么 irq_set_irq_type 要返回错误。

>备注：SMP 系统下，中端有两种形态： 1-N mode，即只有1个 CPU 处理中断；N-N mode，即所有的 CPU 都是独立的收到中断，如果有 N 个 CPU 收到中断，那么就有 N 个处理器来处理该中断。在 GIC 中， SPI 使用 1-N 模型，PPI 和 SGI使用 N-N 模型。1-N 模型，系统（硬件加上软件）必须保证一个中断被一个 CPU 处理，对于 GIC，一个 SPI 的中断可以 trigger 多个 CPU 的 irq line（如果 Distributor 中的 Interrupt Processor Targets Registers 有多个 bit 被设定）但是，该interrupt source和CPU的接口寄存器（例如 ack register ）只有一套，也就是说，这些寄存器接口是全局的，是 global 的，一旦一个 CPU ack（读 Interrupt Acknowledge Register，获取 interrupt ID）了该中断，那么其他的 CPU 看到的该 interupt source 的状态也是已经 ack 的状态。在这种情况下，如果第二个 CPU ack 该中断的时候，将获取一个假的 interrupt ID。对于 PPI 或者 SGI，使用 N-N mode，其 interrupt source 的寄存器是 per CPU 的，也就是每个 CPU 都有自己的、针对该 interrupt source 的寄存器接口（这些寄存器叫做 banked register）。一个 CPU 清除了该 interrupt source 的 pending 状态，其他的 CPU 感知不到这个变化，它们仍然认为该中断是 pending 的。

### irq_set_chip_data

这个函数设置每个 irq chip 的私有数据
```c
int irq_set_chip_data(unsigned int irq, void *data)
{
    unsigned long flags;
    struct irq_desc *desc = irq_get_desc_lock(irq, &flags, 0); 
    desc->irq_data.chip_data = data;
    irq_put_desc_unlock(desc, flags);
    return 0;
}
```

### 设定 high level handler

这是中断处理的核心内容，__irq_set_handler 就是设定 high level handler 的接口函数，不过一般不会直接调用，而是通过 irq_set_chip_and_handler_name 或者 irq_set_chip_and_handler 来进行设定。
```c
void __irq_set_handler(unsigned int irq, irq_flow_handler_t handle, int is_chained, const char *name)
{
    unsigned long flags;
    struct irq_desc *desc = irq_get_desc_buslock(irq, &flags, 0);

    ……
    desc->handle_irq = handle;
    desc->name = name;

    if (handle != handle_bad_irq && is_chained) {
        irq_settings_set_noprobe(desc);
        irq_settings_set_norequest(desc);
        irq_settings_set_nothread(desc);
        irq_startup(desc, true);
    }
out:
    irq_put_desc_busunlock(desc, flags);
} 
```
- is_chained 参数用在 interrupt 级联的情况下，例如中断控制器 B 级联到中断控制器 A 的第 x 个 interrupt source 上。那么对于 A 上的 x 这个 interrupt 而言，在设定其 IRQ handler 参数的时候要设定 is_chained 参数等于1，由于这个 interrupt source 用于级联，因此不能 probe、不能被 request （已经被中断控制器B使用了），不能被 threaded （中断线程化）








 

