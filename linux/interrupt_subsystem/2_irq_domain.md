# IRQ Domain 介绍

## 概述

在linux kernel中，我们使用下面两个ID来标识一个来自外设的中断：

- IRQ number：CPU 需要为每一个外设中断编号，我们称之IRQ Number。这个 IRQ number 是一个虚拟的 interrupt ID，和硬件无关，仅仅是被 CPU 用来标识一个外设中断。 
- HW interrupt ID：对于 interrupt controller 而言，它连接了多个外设的 interrupt request line 并向上传递，因此，interrupt controller 需要对外设中断进行编码。Interrupt controller 用 HW interrupt ID 来标识外设的中断。在 interrupt controller 级联的情况下，仅仅用 HW interrupt ID 已经不能唯一标识一个外设中断，还需要知道该 HW interrupt ID 所属的interrupt controller（HW interrupt ID在不同的Interrupt controller上是会重复编码的）。

这样，CPU 和 interrupt controller 在标识中断上就有了不同的概念。对于驱动工程师，我们和 CPU 视角是一样的，我们只希望得到一个 IRQ number，而不关心具体是哪个 interrupt controller 上的哪个 HW interrupt ID。

因此需要 Linux 中断子系统提供将 HW interrupt ID 映射到 IRQ number 的机制

## 历史

早期系统只有一个 interrupt controller 时，中断控制器上的 HW interrupt ID 可以直接作为 IRQ number

随着系统越来越复杂，外设中断数据增加，系统需要多个中断控制器进行级联，如 root interrupt controller 下连接 GPIO 类型的 interrupt controller，甚至音频系统也需要连接在 GPIO interrupt controller 下的独立 interrupt controller。这就引入了 irq domain（领域）这个概念

irq domain 简单来讲就是将 interrupt controller 树上不同层级上的 interrupt controller 进行区域划分，每一个 interrupt controller 都会对所有连接其上的 interrupt request line 分配 HW interrupt ID，但这个 HW interrupt ID 仅仅限制在本 interrupt controller 上

## IRQ Domain API

### 向系统注册 irq domain

Linux 通用中断处理模块提供了三种 irq domain 注册机制（定义于 linux/irqdomain.h），将HW interrupt ID 和 IRQ number 的映射关系分为三类

- 线性映射
    
    建立一个 lookup table，以 HW interrupt ID 为 index，通过查表获取对应的 IRQ number
    
    对于 Linear map 而言，interrupt controller 对其 HW interrupt ID 进行编码的时候要满足一定的条件：hw ID 不能过大，而且 ID 排列最好是紧密的。
    
    irq_domain_add_linear 函数用于注册线性映射的 domain
    ```c
    static inline struct irq_domain *irq_domain_add_linear(struct device_node *of_node,
					 unsigned int size,
					 const struct irq_domain_ops *ops,
					 void *host_data)
    {
	    return __irq_domain_add(of_node, size, size, 0, ops, host_data);
    }
    ```

- 基数树映射

    建立一个 Radix Tree 来维护 HW interrupt ID到 IRQ number 映射关系，HW interrupt ID 作为 lookup key，在 Radix Tree 检索到 IRQ number。
    
    对于不满足 Linear map 条件的 interrupt controller，可以使用 Radix Tree map （目前只有 powerPC 和 MISP 平台）
    
    irq_domain_add_tree 函数用于注册基数树映射的 domain
    ```c
    static inline struct irq_domain *irq_domain_add_tree(struct device_node *of_node,
					 const struct irq_domain_ops *ops,
					 void *host_data)
    {
	    return __irq_domain_add(of_node, 0, ~0, 0, ops, host_data);
    }
    ```
    
- 不映射

    即直接将 IRQ number 写入 HW interrupt ID 配置寄存器（只有PowerPC 系统使用的MPIC (Multi-Processor Interrupt Controller)支持这种方式）
    
    irq_domain_add_nomap 函数用于注册不映射的 domain
    ```c
    static inline struct irq_domain *irq_domain_add_nomap(struct device_node *of_node,
					 unsigned int max_irq,
					 const struct irq_domain_ops *ops,
					 void *host_data)
    {
	    return __irq_domain_add(of_node, 0, max_irq, max_irq, ops, host_data);
    }
    ```

__irq_domain_add 最终会将 irq domain 挂入 irq_domain_list 的全局列表

### 为 irq domain 创建映射

创建 irq domain 后，就需要在 irq domain 中建立 HW interrupt ID 和  IRQ number 的映射关系

Linux 通用中断处理模块提供了四个建立映射函数

- irq_create_mapping

    irq_create_mapping 函数以 irq domain 和 HW interrupt ID 为参数，返回IRQ number（这个IRQ number是动态分配的）。
    ```c
    extern unsigned int irq_create_mapping(struct irq_domain *host, irq_hw_number_t hwirq); 
    ```
    
    这个函数适用于 HW interrupt ID 已知的中断控制器，如 GPIO，它的 HW interrupt ID 和 GPIO 有着特定的关系

- irq_create_strict_mappings

    irq_create_strict_mappings 函数用来为一组 HW interrupt ID 建立映射
    ```c
    extern int irq_create_strict_mappings(struct irq_domain *domain, unsigned int irq_base, irq_hw_number_t hwirq_base, int count);  
    ```
    
- irq_create_of_mapping

    irq_create_of_mapping 函数利用device tree进行映射关系的建立
    ```c
    extern unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data);
    ```
    
    通常，一个普通设备的 device tree node 已经描述了足够的中断信息，在这种情况下，该设备的驱动在初始化的时候可以调用 irq_of_parse_and_map 函数分析该 device node 中和中断相关的内容（interrupts 属性），并建立映射关系
    ```c
    unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
    {
        struct of_phandle_args oirq;

        if (of_irq_parse_one(dev, index, &oirq))    /* 分析device node中的 interrupts 属性 */
            return 0;

        return irq_create_of_mapping(&oirq);        /* 创建映射，并返回对应的IRQ number */
    }
    ```

    推荐使用使用 Device tree 来创建映射，这样的驱动程序基本上初始化需要调用 irq_of_parse_and_map 获取 IRQ number，然后调用 request_threaded_irq 申请中断 handler。
    
- irq_create_direct_mapping

    用于支持 no map 类型 irq domain 的 interrupt controller
    
## 相关数据结构

### irq_domain_ops 结构体

注册 irq domain 时，需要通过一个 irq_domain_ops 结构体指针，提供一组回调函数
```c
struct irq_domain_ops {
    int (*match)(struct irq_domain *d, struct device_node *node);
    int (*map)(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw);
    void (*unmap)(struct irq_domain *d, unsigned int virq);
    int (*xlate)(struct irq_domain *d, struct device_node *node, const u32 *intspec, unsigned int intsize, unsigned long *out_hwirq, unsigned int *out_type);
}; 
```
- xlate 函数用于在 DT 中指定节点（node 参数指定）的多个（initsize 参数指定） interrupts 属性 翻译成 HW interrupt ID（out_hwirq 参数）和 trigger （out_type）
- match 函数用于判断指定的 interrupt controller（node参数）是否和指定的 irq domain匹配（d参数），如果匹配的话，返回1。不过一般不需要提供这个函数，因为 struct irq_domain 中有一个 of_node 指针指向了对应的 interrupt controller 的device node，而且有默认的匹配函数来判断 of_node 成员是否等于传入的 node 参数。
- map 函数在创建或更新 HW irterrput ID - Interrupt ID 映射关系时调用，用于设定该 IRQ number 对应中断描述符（struct irq_desc）的  irq chip、irq chip data、highlevel irq-events handler 
- unmap 函数与 map 函数相反，在解除 HW irterrput ID - Interrupt ID 关系时调用

### irq_domain 结构体

在 Linux 内核中，irq domain 由 struct irq_domain 描述
```c
 struct irq_domain{
    struct list_head link;
    const char *name;
    const struct irq_domain_ops *ops;   /* callback函数 */
    void *host_data;                    /* interrupt controller 使用的私有数据 */

    /* Optional data */
    struct device_node *of_node;        /* 该interrupt domain 对应的 interrupt controller 的device node */
    struct irq_domain_chip_generic *gc; /* generic irq chip */

    /* reverse map data. The linear map gets appended to the irq_domain */
    irq_hw_number_t hwirq_max;          /* 该 domain 中最大的那个HW interrupt ID */
    unsigned int revmap_direct_max_irq;
    unsigned int revmap_size;           /* 线性映射的size，for Radix Tree map和no map，该值等于0 */
    struct radix_tree_root revmap_tree; /* Radix Tree map 使用到的 radix tree root node */
    unsigned int linear_revmap[];       /* 线性映射使用的 lookup table */
}; 
```

所有的 irq domain，都使用 link 成员挂入 irq_domain_list 全局列表
```c
static LIST_HEAD(irq_domain_list); 
```

通过 irq_domain_list，可以获取整个系统中 HW interrupt ID 和 IRQ number 的映射关系

对于线性映射：
- linear_revmap 保存了一个线性的 lookup table，index 是 HW interrupt ID，table 中保存了 IRQ number 值。
- revmap_size等于线性的lookup table的size。
- hwirq_max保存了最大的HW interrupt ID。
- revmap_direct_max_irq没有用，设定为0。

对于基数树映射：
- revmap_size 等于0。
- hwirq_max 没有用，设定为一个最大值。 
- revmap_direct_max_irq 没有用，设定为0。
- revmap_tree 指向 Radix tree 的 root node。

## Device Tree 中和中断相关的属性

想要进行映射，首先要了解 interrupt controller 的拓扑结构。系统中的 interrupt controller 的拓扑结构以及其 interrupt request line 的分配情况（分配给哪一个具体的外设）都在 Device Tree 文件中通过interrupt-parent 和 interrupts 属性描述。

对于产生中断的外设，需要定义 interrupt-parent 和 interrupts 属性：

- interrupt-parent：指定该外设的 interrupt request line 物理连接到了哪一个中断控制器上
- interrupts：描述了具体该外设产生的 interrup t的细节信息（也就是 interrupt specifier）。例如：HW interrupt ID（由该外设的 device node 中的 interrupt-parent 指向的 interrupt controller 解析）、interrupt 触发类型等。

对于 interrupt controller，需要定义 interrupt-controller 和 #interrupt-cells 的属性：

- interrupt-controller：表示该 device node 就是一个中断控制器
- \#interrupt-cells：表示该中断控制器用多少个 cell （一个 32-bit 的单元）描述一个外设的 interrupt request line。每个cell表示什么样的含义由 interrupt controller 自己定义。
- interrupts 和 interrupt-parent：对于那些非 root interrupt controller，其本身也是作为一个产生中断的外设连接到其他的 interrupt controller 上，因此也需要定义 interrupts 和 interrupt-parent 属性。

## 建立 Mapping DB

系统中 HW interrupt ID 和 IRQ number 的 mapping DB 是在整个系统初始化的过程中建立起来的，过程如下：

1. DTS 文件描述了系统中的 interrupt controller 以及外设 IRQ 的拓扑结构，在 linux kernel 启动的时候，由 bootloader 传递给 kernel（实际传递的是DTB）
2. 在 Device Tree 初始化的时候，形成了系统内所有的 device node 的树状结构，当然其中包括所有和中断拓扑相关的数据结构（所有的 interrupt controller 的 node 和使用中断的外设 node）
3. 在 machine driver 初始化的时候会调用 of\_irq\_init 函数，在该函数中会扫描所有 interrupt controller 的节点，并调用适合的 interrupt controller driver 的初始化函数。毫无疑问，初始化需要注意顺序，首先初始化 root，然后first level，second level，最后是 leaf node 。在初始化的过程中，一般会调用 irq\_domain\_add_\* 接口函数向系统注册 irq domain。有些 interrupt controller 会在其 driver 初始化的过程中创建映射

### 注册 irq domain

以 GIC 驱动的初始化代码为例（gic_of_init->git_init_bases）
```c
void __init gic_init_bases(unsigned int gic_nr, int irq_start,
               void __iomem *dist_base, void __iomem *cpu_base,
               u32 percpu_offset, struct device_node *node)
{
    irq_hw_number_t hwirq_base;
    struct gic_chip_data *gic;
    int gic_irqs, irq_base, i;

    ……
    /* 对于root GIC */
    hwirq_base = 16;
    gic_irqs -= hwirq_base /* 之所以减去16是因为 root GIC 的 0～15 号 HW interrupt ID 是为 IPI 预留的，因此要去掉。所以 hwirq_base 从 16 开始 */


    irq_base = irq_alloc_descs(irq_start, 16, gic_irqs, numa_node_id()); /* 申请 gic_irqs 个连续的 IRQ number，从16号开始搜索 IRQ number。由于是root GIC，申请的 IRQ 基本上会从16号开始 */

    ……
    
    gic->domain = irq_domain_add_legacy(node, gic_irqs, irq_base,
                    hwirq_base, &gic_irq_domain_ops, gic);  /* 初始化并注册 irq domain，并根据 gic_irqs, irq_base,
                    hwirq_base 创建映射 */

    ……
} 
```

这里通过 irq_domain_add_legacy 将 domain->hwirq_max 和 domain->revmap_size 设置为 hwirq_base + gic_irqs

GIC 驱动并没有使用标准的 irq\_domain\_add_\* 函数注册 irq domain，因为 arch/arm 下仍然重置很多 board specific 代码，定义了很多与设备相关的静态数组，这些数组规定了各个 device 使用的资源，包括 IRQ 资源。这种情况下各个外设的 IRQ 是固定的，也就是说 HW interrupt ID 和 IRQ number 的关系是固定的。一旦关系固定，我们就可以在 interupt controller 的代码中创建这些映射关系。
```c
struct irq_domain *irq_domain_add_legacy(struct device_node *of_node,
                     unsigned int size,
                     unsigned int first_irq,
                     irq_hw_number_t first_hwirq,
                     const struct irq_domain_ops *ops,
                     void *host_data)
{
    struct irq_domain *domain;

    domain = __irq_domain_add(of_node, first_hwirq + size,
                  first_hwirq + size, 0, ops, host_data);   /* 注册irq domain */
    if (!domain)
        return NULL;

    irq_domain_associate_many(domain, first_irq, first_hwirq, size);  /* 在 domain 中创建 size 对映射 */

    return domain;
} 
```

至此，HW interrupt ID 和 IRQ number 的映射关系已经建立，保存在线性 lookup table 中，gic_irqs 等于 GIC 支持的中断数目，具体如下：

index 0～15对应的IRQ无效

16号IRQ  <------------------>16号HW interrupt ID

17号IRQ  <------------------>17号HW interrupt ID 

……

### 创建 HW interrupt ID 和 IRQ number 的映射关系

设备驱动在初始化时，可以使用 irq_of_parse_and_map 函数分析 device_node 的 interrupts 属性，以获取中断信息，并创建映射关系
```c
unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
{
    struct of_phandle_args oirq;

    if (of_irq_parse_one(dev, index, &oirq))    /* 分析 device node 中的 interrupts 属性 */
        return 0;

    return irq_create_of_mapping(&oirq);    /* 创建映射 */
} 
```

这里使用 irq_create_of_mapping 函数在 parent irq domain 中创建映射：
```c
unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data)
{
    struct irq_domain *domain;
    irq_hw_number_t hwirq;
    unsigned int type = IRQ_TYPE_NONE;
    unsigned int virq;

    domain = irq_data->np ? irq_find_host(irq_data->np) : irq_default_domain; /* 通过 interrupt parent 寻找 irq domain ，如果没有则使用 irq_default_domain */
    if (!domain) {
        return 0;
    }

    if (domain->ops->xlate == NULL)     /* domain 没有提供 xlate 函数，则获取 interrupts 数组的第一个元素作为 HW interrupt ID */
        hwirq = irq_data->args[0];
    else {                              /* domain 提供 xlate 函数，则由 domain 负责解析 interrupts 数组，将 HW interrupt ID 和 interrupt type （触发方式等）分别赋值给 hwirq 和 type */
        if (domain->ops->xlate(domain, irq_data->np, irq_data->args,    
                    irq_data->args_count, &hwirq, &type))
            return 0;
    }

    /* Create mapping */
    virq = irq_create_mapping(domain, hwirq);/* 创建 HW interrupt ID 和 IRQ number 的映射关系 */
    if (!virq)
        return virq;

    /* Set type if specified and different than the current one */
    if (type != IRQ_TYPE_NONE &&
        type != irq_get_trigger_type(virq))
        irq_set_irq_type(virq, type);       /* 如果有需要，设置 trigger type  */
    return virq;
} 
```

irq_find_host 函数根据 irq_data 的 np 指针来寻找 irq domain， np 指向 interrupt parent

irq_create_mapping 函数可以使用已经获取到的 HW interrupt ID 和 domain 来创建映射
```c
unsigned int irq_create_mapping(struct irq_domain *domain,
                irq_hw_number_t hwirq)
{
    unsigned int hint;
    int virq;

    /* 如果映射已经存在，直接返回 */
    virq = irq_find_mapping(domain, hwirq);
    if (virq) {
        return virq;
    }


    hint = hwirq % nr_irqs;                        /* 分配一个中断描述符以及对应的 irq number */
    if (hint == 0)
        hint++;
    virq = irq_alloc_desc_from(hint, of_node_to_nid(domain->of_node));
    if (virq <= 0)
        virq = irq_alloc_desc_from(1, of_node_to_nid(domain->of_node));
    if (virq <= 0) {
        pr_debug("-> virq allocation failed\n");
        return 0;
    }

    if (irq_domain_associate(domain, virq, hwirq)) { /* 向 domain 添加 mapping */
        irq_free_desc(virq);
        return 0;
    }

    return virq;
} 
``` 

最后使用 irq_domain_associate 函数建立 mapping
```c
int irq_domain_associate(struct irq_domain *domain, unsigned int virq,
             irq_hw_number_t hwirq)
{
    struct irq_data *irq_data = irq_get_irq_data(virq);
    int ret;

    mutex_lock(&irq_domain_mutex);
    irq_data->hwirq = hwirq;
    irq_data->domain = domain;
    if (domain->ops->map) {
        ret = domain->ops->map(domain, virq, hwirq);    /* 调用 irq domain 的 map 回调函数 */
    }

    if (hwirq < domain->revmap_size) {
        domain->linear_revmap[hwirq] = virq;            /* 填写线性映射 lookup table 的数据 */
    } else {
        mutex_lock(&revmap_trees_mutex);
        radix_tree_insert(&domain->revmap_tree, hwirq, irq_data);   /* 向 radix tree 插入一个 node */
        mutex_unlock(&revmap_trees_mutex);
    }
    mutex_unlock(&irq_domain_mutex);

    irq_clear_status_flags(virq, IRQ_NOREQUEST);        /* 该 IRQ 已经可以申请了，因此 clear 相关 flag */

    return 0;
} 
```

## 将 HW interrupt ID 转换成 IRQ number

创建了庞大的 HW interrupt ID 到 IRQ number 的 mapping DB ，最终还是要使用。具体的使用场景是在 CPU 相关的处理函数中，程序会读取硬件 interrupt ID，并转成 IRQ number，调用对应的 irq event handler。这里以一个级联的 GIC 系统为例，描述转换过程

### GIC driver 初始化
second GIC 的初始化
```c
void __init gic_init_bases(unsigned int gic_nr, int irq_start,
               void __iomem *dist_base, void __iomem *cpu_base,
               u32 percpu_offset, struct device_node *node)
{
    irq_hw_number_t hwirq_base;
    struct gic_chip_data *gic;
    int gic_irqs, irq_base, i;

    ……
    /* 对于second GIC */
    hwirq_base = 32; 
    gic_irqs -= hwirq_base /* 之所以减去 32 主要是因为对于 second GIC，其0～15号 HW interrupt 是 for IPI 的，因此要去掉。而16～31号 HW interrupt 是 for PPI 的，也要去掉。也正因为如此 hwirq_base 从 32 开始 */


    irq_base = irq_alloc_descs(irq_start, 16, gic_irqs, numa_node_id()); /* 申请 gic_irqs 个 IRQ 资源，从 16 号开始搜索 IRQ number。由于是 second GIC，申请的 IRQ 基本上会从 root GIC 申请的最后一个 IRQ 号＋1开始 */


    gic->domain = irq_domain_add_legacy(node, gic_irqs, irq_base,
                    hwirq_base, &gic_irq_domain_ops, gic);  /* 向系统注册irq domain并创建映射 */

    ……
} 
```

second GIC初始化之后，该irq domain的HW interrupt ID和IRQ number的映射关系已经建立，保存在线性lookup table中，size等于GIC支持的中断数目，具体如下：

index 0～32对应的IRQ无效

root GIC申请的最后一个IRQ号＋1  <------------------>32号HW interrupt ID

root GIC申请的最后一个IRQ号＋2  <------------------>33号HW interrupt ID

……

回到 git_of_init 函数，对于 second GIC， 还有其他的初始化内容：
```c
int __init gic_of_init(struct device_node *node, struct device_node *parent)
{

    ……

    if (parent) {
        irq = irq_of_parse_and_map(node, 0);    /* 解析 second GIC 的 interrupts 属性，并进行 mapping，返回 IRQ number */
        gic_cascade_irq(gic_cnt, irq);          /* 设置 handler */
    }
    ……
} 
```
对于 second GIC，它是作为其 parent（root GIC）的一个普通的 irq source，因此，也需要注册该 IRQ 的 handler。由此可见，非 root 的 GIC 的初始化分成了两个部分：一部分是作为一个 interrupt controller，执行和 root GIC 一样的初始化代码。另外一方面，GIC 又作为一个普通的 interrupt generating device，需要象一个普通的设备驱动一样，注册其中断 handler。

gic_cascade_irq函数如下： 
```c
void __init gic_cascade_irq(unsigned int gic_nr, unsigned int irq)
{
    if (irq_set_handler_data(irq, &gic_data[gic_nr]) != 0)  /* 设置handler data */
        BUG();
    irq_set_chained_handler(irq, gic_handle_cascade_irq);   /* 设置 handler */
} 
```

### 在中断处理过程中，将 HW interrupt ID 转成 IRQ number

在系统的启动过程中，经过了各个 interrupt controller 以及各个外设驱动的努力，整个 interrupt 系统的 database（将 HW interrupt ID 转成 IRQ number 的数据库）已经建立。一旦发生硬件中断，执行过 CPU architecture 相关的中断代码之后，会调用 irq handler，该函数的一般过程如下：

1. 首先找到 root interrupt controller 对应的 irq domain。
2. 根据 HW 寄存器信息和 irq domain 信息获取 HW interrupt ID
3. 调用 irq_find_mapping 找到 HW interrupt ID 对应的 irq number
4. 调用 handle_IRQ（对于ARM平台）来处理该 irq number

对于级联的情况，过程类似上面的描述，但是需要注意的是在步骤4中不是直接调用该 IRQ 的 hander 来处理该 irq number，因为，这个 irq 需要各个 interrupt controller level 上的解析。举一个简单的二阶级联情况：假设系统中有两个 interrupt controller A 和 B，A 是 root interrupt controller，B 连接到 A 的 13 号 HW interrupt ID 上。在 B interrupt controller 初始化的时候，除了初始化它作为 interrupt controller 的那部分内容，还有初始化它作为 root interrupt controller A 上的一个普通外设这部分的内容。最重要的是调用 irq_set_chained_handler 设定 handler。这样，在上面的步骤4的时候，就会调用 13 号 HW interrupt ID 对应的 handler（也就是B的 handler），在该 handler 中，会重复上面的（1）～（4）。