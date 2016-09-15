# 第八章 内存分配

## kmalloc

## 后备告诉缓存

## 内存池

## get_free_page 系列函数

## alloc_pages 接口

## vmalloc 和其相关函数

## Pre-CPU 变量

## 获取大量内存

## kmalloc
kmalloc 不会清除原先内存内容

kmalloc 分配的物理内存是连续的, 如果没有租都的连续内存空间, kmalloc 有可能会停顿

kmalloc 定义于 linux/slab.h
```c
void *kmalloc(size_t size, int flags); 
```

### flags 参数
flags 参数用于控制 kmalloc 的行为

常用的 flags 值(定义在 linux/gfp.h):
- GFP_KERNEL

    表示代替当前正在内核空间中的进程分配内核内存; 使用此 flags 意味着 kmalloc 在可用内存不足时, 有权将当前进程推入休眠状态; 使用此 flags 的函数必须时可重入的, 而且不能在原子环境下
    
- GFP_ATOMIC
    
    用于中断操作或其他非进程环境的程序; 使用此 flag 时的函数是不能使当前进程休眠的; 使用此 flag 时, kmalloc 函数有权用掉最后一页内存
    
- GFP_USR

    用于分配内存给用户空间进程, 有可能休眠
    
- GFP_HIGHUSR
    
    从高地址区分配内存给用户空间进程
    
- GFP_NOIO 和 GFP_NOFS

    类似于 GFP_KERNEL, 但是增加了一些功能限制: NOIO 禁止触发任何 I/O 操作; NOFS 不允许任何文件系统调用. 它们主要用于文件系统和虚拟内存的实现中, 允许休眠; 尽量少用递归的文件系统调用
    
- __GFP_DMA

    分配可用于 DMA 传输的内存
    
- __GFP_HIGHMEM

    分配高地址区的内存, 在没有高地址区的平台上, 此 flag 无意义
    
- __GFP_COLD

    与其他 flag 不一样, 此 flag 要求分配一些有一段时间没被使用的内存, 一般用于分配给 DMA read 使用
    
- __GFP_NOWARN

    很少使用, 可避免内核在无法满足配置要求时发出警告(printk)

- __GFP_HIGH

    要求内核不急代价满足配置要求, 即使要使用原本保留给应付紧急情况的内存
    
- __GFP_NORETRY 

    当无法满足配置要求时, 此 flag 要求 kmalloc 立即放弃
    
- __GFP_REPEAT
    
    当无法满足配置要求时, 此 flag 要求 kmalloc 再试一次
    
- __GFP_NOFAIL
    
    当无法满足配置要求时, 此 flag 要求 kmalloc 乌璐如何都要满足要求, 一般不被使用

> 备注: __* 的 flag 可以和其他 flag 结合使用

### 内存划分
Linux 内存一般划分为 正常、 DMA-capable、 高地址区; 

一般的分配操作都是从正常分区中分配空间

DMA 区是一段预先划分好的内存, 用于周边设备进行 DMA 访问; x86 系统上, 前16MB RAM 可配合ISA设备的 DMA 操作, PCI 设备则没这个限制

高地址区是一种可供 32-bit 平台访问相对大量内存的机制. 高地址区的内存不可被直接访问, 必须实现将其映射到一段特殊的地址范围

内核构建一个内存分区列表, 当调用 kmalloc 时, 便会在这个列表中寻找

配置要求指定 __GFP_DMA flag 时, 只查找是否有可用的 DMA 内存, 如果低地址区已经耗尽, 则配置失败; 如果没指定特殊的 flag, 则只查找正常区和DMA区; 如果指定 __GFP_HIGHMEM flag, 则会查找全部分区

### size 参数
Linux 内核采取面向分页配置的计数, 这个计数定义了几种不同大小的分页, 相同大小的分页被集中于一个分页池中; 需要分配特定的内存是, 就草有足够容量的分页池中, 将整块内存分配出去

Linux 核心只能配置集中预定容量的字节数组, 如果没有合适的容量, 则需要选择大一级的容量; kmalloc 函数最小的配置单位是32 或 64 bytes(视内存分页大小而定)

size 值的上限取决于硬件平台以及核心配置项, 不过为了确保程序的可移植性, 取值最好不要超过128KB

## 后备高速缓存
后备高速缓存用于将一些高需求量的内存分页集中在一起, 这样有助于提升内存访问的效率

### 后备高速缓存 API
Linux 内核里的缓存管理系统 slab allocator (定义于 linux/slab.h)提供了对后备高速缓存的支持

高速缓存使用结构体 kmem_cache_t 描述

kmem_cache_create 函数用于创建 kmem_cache_t
```c
kmem_cache_t *kmem_cache_create(const char *name, size_t size, size_t offset, unsigned long flags, void (*constructor)(void *, kmem_cache_t *, unsigned long flags), void (*destructor)(void *, kmem_cache_t *, unsigned long flags));
```
- 字串 name 定义缓存名
- size 定义缓存中每块内存的大小
- offset 定义第一块内存在内存分页里的起始位置, 0 表示使用默认值
- flags 描述控制方式, 常用取值如下, 其他用于除错缓存配置操作的 flag 定义于 mm/slab.h(这些选项通常通过.config 来设定)
    - SLAB_NO_REAP: 保护缓存不会因为系统内存短缺而减少其容量, 一般不使用
    - SLAB_HWCACHE_ALIGN: 要求将每一个数据对象都对齐到一个缓存行, 实际对齐方式视目标平台的缓存布局方式而定; 通常用于SMP系统上经常被访问的缓存, 但是因对齐锁造成的空缺也会形成浪费
    - SLAB_CACHE_DAM: 要求每一个数据对象都配置在 DMA-capable 内存区中
- constructor 和 destructor 函数指针指向对象初始化和清理对象内容的函数， 这两个函数会被 slab allocator 自动调用; 其 flags 参数为 SLAB_CTOR_CONSTRUCTOR 时, 表示调用 constructor, 否则为调用 destructor; flags 参数为 SLAB_CTOR_ATOMIC 时, 表示 constructor 和 destructor 不能休眠

kmem_cache_alloc 函数用于从高速缓存中分配内存
```c
void *kmem_cache_alloc(kmem_cache_t *cache, int flags);
```
- flags 参数与 kmalloc 的 flags 参数相同

kmem_cache_free 函数将内存回收回高速缓存
```c
void kmem_cache_free(kmem_cache_t *cache, const void *mem); 
```

kmem_cache_destroy 函数用于释放高速缓存
```c
int kmem_cache_destroy(kmem_cache_t *cache); 
```
- 只有所有分配出去的内存都被回收, 释放高速缓存才会成功; 返回非0值时, 需要检查内存是否全部回收(即存在内存泄露)

## 内存池
内存池总是会维护一些预留内存, 以备紧急情况下使用. 从内存池分配内存是不会失败的

### 内存池 API
内存池使用 mempool_t 结构体描述(定义于 linux/mempool.h)

mempool_create 函数用于创建内存池
```c
mempool_t *mempool_create(int min_nr, mempool_alloc_t *alloc_fn, mempool_free_t *free_fn, void *pool_data); 
```
- min_nr 表示内存池中预留内存的最小数量
- alloc_fn 和 free_fn 函数在分配或回收内存时会被调用
```c
typedef void *(mempool_alloc_t)(int gfp_mask, void *pool_data);
typedef void (mempool_free_t)(void *element, void *pool_data);
```
- pool_data 参数会传递给 alloc_fn 和 free_fn 函数
- linux 内核的 slab allocator 提供了一套内存分配和回收函数 mempool_alloc_slab 和 mempool_free_slab, 可以在 mempool_create 函数中直接使用
```c
cache = kmem_cache_create(. . .); 
pool = mempool_create(MY_POOL_MINIMUM,mempool_alloc_slab, mempool_free_slab, cache); 
```

内存池创建后, 先创建预留内存, 然后才可以使用 mempool_alloc 函数

mempool_alloc 和 mempool_free 函数用于分配和回收内存
```c
void *mempool_alloc(mempool_t *pool, int gfp_mask);
void mempool_free(void *element, mempool_t *pool);
```
- gfp_mask 参数与 kmalloc 的 flags 参数相同
- mempool_alloc 函数会尝试调用内存分配函数获取非内存池维护的内存; 若分配失败, 则返回一个内存池中的一块预留内存(如果还有剩余)
- mempool_free 回收内存时, 如果内存池中预留内存的数量少于最低数量, 内存由内存池回收, 否则由系统回收

mempool_resize 函数可以重置内存池的预留内存数
```c
int mempool_resize(mempool_t *pool, int new_min_nr, int gfp_mask);
```

mempool_destory 函数用于释放内存池
```c
void mempool_destroy(mempool_t *pool); 
```
>备注:
驱动程序一般不使用内存池, 除非不允许内存分配失败

## get_free_page 系列函数
如果模块需要比较大的内存, 通常按页分配

相关函数和宏在linux/gfp.h中定义
```c
unsigned long get_zeroed_page(unsigned int flags);
unsigned long __get_free_page(unsigned int flags);
unsigned long __get_free_page(unsigned int flags, unsigned int order);
```
- order 是要申请页面数的以2为底的对数
- 参数flags和kmalloc函数中的一样

free_page 函数用于回收不需要的内存分页
```c
void free_page(unsigned long addr);  
void free_pages(unsigned long addr, unsigned long order);
```

## alloc_pages 接口
Linux 内核使用 struct page 结构体描述内存分页的数据结构, 内核经常需要直接操作 page 结构, 如处理高地址区时, 因为高地址区在内核空间里没有固定的地址

alloc_pages 接口(定义于linux/gfp.h)可以返回 page 结构体
```c
struct page *alloc_pages_node(int nid, unsigned int flags, unsigned int order);
struct page *alloc_pages(unsigned int flags, unsigned int order);
struct page *alloc_page(unsigned int flags);
```
- nid 是 NUMA 节点识别码, 一般使用 numa_node_id 函数生成
- flags 和 order 与 __get_free_page 相同

free_page 函数用来销毁 page 结构体
```c
void __free_page(struct page *page);
void __free_pages(struct page *page, unsigned int order);
void free_hot_page(struct page *page);
void free_cold_page(struct page *page);
```
- 当特定内存分页的内容常驻于处理器的缓存时, 应使用 free_hot_page 函数, 如果不常驻, 应使用 free_code_page 函数

## vmalloc 和其相关函数
vmalloc 函数的作用是在虚拟地址空间分配一段连续内存

虚拟地址连续不代表物理内存区域也是连续的

通过 vmalloc 获得的内存使用起来不是很高效

vmalloc 函数在 linux/vmalloc.h 中定义
```c
#include <linux/vmalloc.h> 
void *vmalloc(unsigned long size);
void vfree(void * addr);
void *ioremap(unsigned long offset, unsigned long size);
void iounmap(void * addr);
```

kmalloc 和 get_free_pages 函数返回的虚拟内存地址与物理内存地址是一一对应关系，所以内核在分配内存的时候没有修改分页表; 而 vmalloc 和 ioremap 函数返回的虚拟内存地址是模拟的，每一次分配操作都需要修改分页表

iormap 函数只返回虚拟内存地址, 并不分配内存, 所以不能直接访问这个地址, 需要通过 readb 函数 和 I/O 函数来达成

vmalloc 不能用于中断处理, 因为其内部使用了 kmalloc 来获得分页表的存储空间, 可能导致休眠

## Pre-CPU 变量
Pre-CPU 变量是指从属于每个CPU的变量, 简称从属变量。
- Pre-CPU 变量访问时不需要锁定
- 可以将Pre-CPU 变量放在所属处理器的缓存中

DEFINE_PRE_CPU 宏用于声明丛书变量(定义于 linux/precpu.h)
```c
DEFINE_PER_CPU(type, name);
```

get_cpu_var put_cpu_var pre_cpu 用于访问从属变量
```c
get_cpu_var(var)
put_cpu_var(var)
pre_cpu(var, int cpu_id)
```
- get_cpu_var 用于读取从属变量
- put_cpu_var 用于修改从属变量
- pre_cpu 用于读取其他cpu的从属变量, 读取其他cpu的丛书变量时需要加锁

EXPORT_PER_CPU_SYMBOL 宏用于将从属变量共享给其他模块
```c
EXPORT_PER_CPU_SYMBOL(per_cpu_var);
EXPORT_PER_CPU_SYMBOL_GPL(per_cpu_var);
```

如果需要访问其他模块共享的从属变量, 需要先使用 DECLARE_PRE_CPU 声明
```c
DECLARE_PER_CPU(type, name); 
```

## 获取大量内存
要获取大量连续的物理内存, 唯一的机会就是在系统开机期间, 这需要将模块编译进内核

alloc_bootmem 函数(定义于 linux/bootmem.h)用于在开机时分配内存
```c
void *alloc_bootmem(unsigned long size);
void *alloc_bootmem_low(unsigned long size);
void *alloc_bootmem_pages(unsigned long size);
void *alloc_bootmem_low_pages(unsigned long size);
```

free_bootmem 用于释放内存
```c
void free_bootmem(unsigned long addr, unsigned long size); 
```