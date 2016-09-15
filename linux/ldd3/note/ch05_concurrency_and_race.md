# 第五章 并发和竞态

## 并发以及并发控制

内核程序并发控制的原则:
- 驱动程序的设计应尽量避免资源共享, 比如尽量避免使用全局变量
- 对于需要共享的资源, 如硬件设备、内核传递过来的指针等, 使用时必须加锁
- 被共享的资源对象必须持续存在, 直到这个对象不再被使用
- 操作共享资源的代码只能被一个执行单元执行, 这段代码被称为临界区

## 信号量 和 互斥

在计算机科学中， 信号量是一个整数值与一对 P 函数 和 V 函数的组合, 当一个进程即将进入特殊代码时, 必须先调用 P 函数, 如果信号量值大于0, 则将该值减一, 然后进程执行特殊的代码, 如果信号量值为0(或小于0), 则进程必须等待其他进程释放信号量。 当进程离开特殊代码时, 必须调用 V 函数来释放信号量, V 函数会将信号量值加一

当信号量值被初始化为1, 代表信号量被用于排他独占--避免有多个进程同时进入特殊代码, 这样的信号量被称为互斥

### Linux 信号量机制
使用 Linux 信号量机制需要引入 linux/semaphore.h， 定义并初始化 semaphore 结构体
```c
#include <linux/semaphore.h>
struct semaphore sem;
static inline void sema_init(struct semaphore *sem, int val)
```

down 函数会递减信号量值, 用于获取信号量, 如果没有获得信号量, 进程会阻塞, 等待信号量被释放
```c
void down(struct semaphore *sem);
int down_interruptible(struct semaphore *sem);
int down_trylock(struct semaphore *sem);
返回值: 成功获取信号量则返回0
```
- down 函数不能被中断打断, 如果 down 函数调用时进程被阻塞, 中断无法将进程唤醒
- down_interruptible 函数可以被中断打断, 如果 down_interruptible 函数调用时进程被阻塞, 中断可以将进程唤醒。此时 down 函数返回非0值
- down_trylock 函数调用时, 无论是否获取信号量, 都立即返回, 成功获取信号量则返回0, 否则返回非0值

up 函数会递增信号量值, 用于释放信号量
```c
void up(struct semaphore *sem);
```

### Linux 互斥机制
使用 Linux 互斥机制需要引入 linux/mutex.h, 定义并初始化 mutex 结构体
```c
# include <linux/mutex.h>
struct mutex m;

mutex_init(&m);
DEFINE_MUTEX(m);
```

mutex_lock 用于获取互斥体
```c
void mutex_lock(struct mutex *lock)
int mutex_lock_interruptible(struct mutex *lock)
int mutex_trylock(struct mutex *lock)
```
- mutex_lock 不可被中断打断
- mutex_lock_interruptible 可以被中断打断
- mutex_trylock 如果没有获取到互斥体, 则直接返回

unlock 用于释放互斥体
```c
void mutex_unlock(struct mutex *lock)
```

### 读写信号量
信号量赋予所有执行单元同样的独占权, 不过有时又只需要单独对读取或写入的动作进行保护, 读写信号量提供了这样的机制

一般允许多个执行单元同时获取读信号量, 但只允许一个执行单元获得写信号量, 且读信号量和写信号量是互斥的, 当读信号量被占用时, 获取写信号量时会被阻塞

使用读写信号量机制需要引入 linux/rwsem.h, 定义并初始化 rw_semaphore 结构体
```c
# include <linux/rwsem.h>
struct rw_semaphore rw_sem;

init_rwsem(rw_sem);
```

down_read up_read 函数用于获取和释放读信号量
```c
void down_read(struct rw_semaphore *sem);
int down_read_trylock(struct rw_semaphore *sem);
void up_read(struct rw_semaphore *sem);
```

down_write up_write 函数用于获取和释放写信号量
```c
void down_write(struct rw_semaphore *sem);
int down_write_trylock(struct rw_semaphore *sem);
void up_write(struct rw_semaphore *sem);
```

## Completion 机制
Completion 用于在一个执行单元的工作完成时, 通知其他执行单元

```c
# include <linux/completion.h>
struct completion c;
struct completion {
	unsigned int done;
	wait_queue_head_t wait;
};

static inline void init_completion(struct completion *c);
DECLARE_COMPLETION(c);
```
wait_for_completion 函数用于等待 completion 事件

wait_for_completion 通过判断 completion.down 是否为0来判断是否阻塞, 当 completion.down 为 0, wait_for_completion 会被阻塞

每调用一次 wait_for_completion， completion.down 都会减一, 直到其等于0
```c
void wait_for_completion(struct completion *c);
int wait_for_completion_interruptible(struct completion *c);
- wait_for_completion 是不可被中断的
- wait_for_completion_interruptible 是可以被中断的
```

complete 函数用于发出 completion 事件
```c
void complete(struct completion *);
void complete_all(struct completion *);
```
- complete 函数只会唤醒一个等待中的执行单元, 调用时会将 completion.down 加 1
- complete_all 会唤醒所有等待中的执行单元, 调用时会将 completion.down 设置为 unsigned int 的最大值 2147483647, 也就是说 complete_all 可以唤醒 2147483647 个等待中的执行单元

completion 通常只用一次, 不过只要处理得当, 也可以被复用（只要不使用complete_all）

对于驱动程序而言, completion 机制的典型用途是在模块结束之前等待内核线程完成其工作(如卸载设备时等待对设备的读写线程完成), 在卸载设备前, 清理函数可要求内核线程结束, 然后等待其完工; 内核线程完工后, 可调用 complete_and_exit 函数通知清理函数并结束内核线程

`<kernel/exit.c>`
```c
NORET_TYPE void complete_and_exit(
    struct completion *comp,
    long code)
{
    if (comp)
        complete(comp);

    do_exit(code);
}
```

## 自旋锁
当自旋锁被获取, 其他申请自旋锁的执行单元会在一个循环中不断扫描自旋锁, 直到该自旋锁被释放

相对于信号量, 自旋锁可用于不得休眠的执行单元, 比如中断处理

自旋锁适用于占用锁时间极短的情况

自旋锁只有两种状态: 1 和 0, 所以通常使用一个 int 值来实现自旋锁, 0 代表已锁定, 1 代表已解锁

已经获取的自旋锁的执行单元再次获取该锁(自旋锁被递归使用), 会导致死锁

执行单元获取自旋锁后如果发生阻塞也会导致死锁, 所以获取自旋锁后尽量不要使用 copy_from_user、copy_to_user 和 kmalloc 等函数

自旋锁是针对多任务系统(比如多处理器系统或在单处理器系统上运行抢占式内核)设计的， 如果不支持多任务系统, 自旋锁将退化为空操作

### 自旋锁 API
使用自旋锁需引入 linux/spinlock.h, 定义并初始化 spinlock_t 结构体
```c
#include <linux/spinlock.h>
DEFINE_SPINLOCK(lock);
void spin_lock_init(spinlock_t *lock);
```
DEFINE_SPINLOCK 和 spin_lock_init 这两个宏用于定义和初始化 spinlock_t
- DEFINE_SPINLOCK 负责定义并初始化
- spin_lock_init 只负责初始化

spin_lock 函数用于获取自旋锁
```c
void spin_lock(spinlock_t *lock);
void spin_lock_irq(spinlock_t *lock);
void spin_lock_bh(spinlock_t *lock);
void spin_lock_irqsave(spinlock_t *lock, unsigned long flags);
```
- spin_lock_irq 会忽略中断
- spin_lock_irqsave 会将当前cpu的状态存储于 flags 中
- spin_lock_bh 关闭底半部

调用 spin_trylock 函数如果不能获取自旋锁, 则返回0; 成功返回非0值
```c
int spin_trylock(spinlock_t *lock);
int spin_trylock_irq(spinlock_t *lock);
int spin_trylock_bh(spinlock_t *lock);
int spin_trylock_irqsave(spinlock_t *lock, long flags);
```

spin_unlock 函数用于释放自旋锁
```c
void spin_unlock(spinlock_t *lock);
void spin_unlock_irq(spinlock_t *lock);
void spin_unlock_bh(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);
```
- spin_unlock_irq 释放使用 spin_lock_irq 获取的自旋锁
- spin_unlock_bh 释放使用 spin_lock_bh 和 spin_trylock_bh 获取的自旋锁
- spin_unlock_irqrestore 释放使用 spin_unlock_irqrestore 和 spin_trylock_irqrestore 获取的自旋锁， spin_lock_irqsave 或 spin_trylock_irqsave 与 spin_unlock_irqrestore 必须在同一个函数内被调用

### 读写自旋锁
同读写信号量, 一般允许多个执行单元同时获取读自旋锁, 但只允许一个执行单元获得写自旋锁, 且读自旋锁和写自旋锁是互斥的, 当读自旋锁被占用时, 获取写自旋锁时会被阻塞

定义和初始化读写自旋锁
```c
#include <linux/spinlock.h>
void rwlock_init(rwlock_t *lock);
DEFINE_RWLOCK(lock);
```

获取读自旋锁
```c
void read_lock(rwlock_t *lock);
void read_lock_irqsave(rwlock_t *lock, unsigned long flags);
void read_lock_irq(rwlock_t *lock);
void read_lock_bh(rwlock_t *lock);
int read_trylock(rwlock_t *lock);
```

释放读自旋锁
```c
void read_unlock(rwlock_t *lock);
void read_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
void read_unlock_irq(rwlock_t *lock);
void read_unlock_bh(rwlock_t *lock);
```

获取写自旋锁
```c
void write_lock(rwlock_t *lock);
void write_lock_irqsave(rwlock_t *lock, unsigned long flags);
void write_lock_irq(rwlock_t *lock);
void write_lock_bh(rwlock_t *lock);
int write_trylock(rwlock_t *lock);
```

释放写自旋锁
```c
void write_unlock(rwlock_t *lock);
void write_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
void write_unlock_irq(rwlock_t *lock);
void write_unlock_bh(rwlock_t *lock);
```

## 约定俗成的规则
- 锁被递归使用, 会导致死锁
- 获取锁后如果发生阻塞也会导致死锁
- 只有供驱动程序内部使用的静态函数可以在被调用时获取锁, 可被外部触发的函数(如read, write)则必须在其内部获取锁
- 如果某种运算设计两个不同的资源, 而每个资源都有各自的锁, 那么所有执行单元都要按照相同的顺序获取获取这两个锁
- 可以先使用一个锁保护所有资源(粗糙锁定), 然后逐渐改用较细腻的锁保护机制(细腻锁定)

## 锁定机制的使用方法

### 无锁算法
环形队列可以在不加锁的情况下, 支持一对 reader-writer 操作
- 只有 writer 有权限修改 write 指针, 而且 writer 必须先写入数据, 然后再移动 write 指针
- 只有 reader 有权限修改 read 指针
- 当 write 指针和 read 指针指向同一块内存, 代表队列为空
- 当 read 指针指向 write 指针指向的下一块内存, 代表队列已满

！(环形队列)[../img/ldd3-5-1.png]

Linux 内核提供通用的环形队列实现, 具体参考 linux/kfifo.h

### 原子操作
有时需要共享的资源只是一个简单的整数值, 为整数值加锁不是明智之举, 要保证这种简单运算不受干扰, Linux 内核提供原子操作机制

#### 32 位原子操作
使用32 位原子操作机制, 需要将变量声明为 atomic_t (在 linux/types.h 中定义)
```c
#include <linux/types.h>
atomic_t v;
```
atomic_t 包含的数据最好不超过3个字节

所有原子操作宏在 asm/atomic.h 中定义
```c
ATOMIC_INIT(int i);                                     /* 初始化atomic_t 为 i */   
int atomic_read(atomic_t *v);                           /* 读取 v 的值 */ 
void atomic_set(atomic_t *v, int i);                    /* 将 v 的值设置为 i */ 
void atomic_add(int i, atomic_t *v);                    /* v 加 i */ 
void atomic_sub(int i, atomic_t *v);                    /* v 减 i */ 
void atomic_inc(atomic_t *v);                           /* v 加 1 */ 
void atomic_dec(atomic_t *v);                           /* v 减 1 */ 
int atomic_sub_and_test(int i, atomic_t *v);            /* v 减 i, 然后判断 v 的值, 如果 v 的值为 0, 则返回非 0 值(true), 否则返回 0(false) */ 
int atomic_inc_and_test(atomic_t *v);                   /* v 加 1, 然后判断 v 的值, 如果 v 的值为 0, 则返回非 0 值(true), 否则返回 0(false) */    
int atomic_dec_and_test(atomic_t *v);                   /* v 减 1, 然后判断 v 的值, 如果 v 的值为 0, 则返回非 0 值(true), 否则返回 0(false) */ 
void atomic_add_negative(int i, atomic_t *v);           /* v 加 i, 然后判断 v 的值, 如果 v 的值为负数, 则返回非 0 值(true), 否则返回 0(false) */ 
void atomic_add_return(int i, atomic_t *v);             /* v 加 i 并返回结果*/ 
void atomic_sub_return(int i, atomic_t *v);             /* v 减 i 并返回结果*/ 
void atomic_inc_return(atomic_t *v);                    /* v 加 1 并返回结果*/ 
void atomic_dec_return(atomic_t *v);                    /* v 减 1 并返回结果*/ 
void atomic_add_unless(atomic_t *v, int a, int u);      /* 如果 v 的值不等于 u, 则 v 加 a, 并返回非 0 值(true), 否则 v 值不变, 并返回 0(false) */ 
void atomic_inc_not_zero(atomic_t *v);                  /* 如果 v 的值不等于 0, 则 v 加 1, 并返回非 0 值(true), 否则 v 值不变, 并返回 0(false) */ 
```

#### 64 位原子操作
使用64 位原子操作机制, 需要将变量声明为 atomic64_t (在 linux/types.h 中定义)
```c
#include <linux/types.h>
atomic64_t v;
```
atomic64_t结构体 使用 u64 存储数据, u64 为 long long 类型, 占 8 个字节

所有原子操作宏在 asm/atomic.h 中定义
```c
ATOMIC64_INIT(int i);                                     /* 初始化atomic_t 为 i */   
int atomic64_read(atomic_t *v);                           /* 读取 v 的值 */ 
void atomic64_set(atomic_t *v, int i);                    /* 将 v 的值设置为 i */ 
void atomic64_add(int i, atomic_t *v);                    /* v 加 i */ 
void atomic64_sub(int i, atomic_t *v);                    /* v 减 i */ 
void atomic64_inc(atomic_t *v);                           /* v 加 1 */ 
void atomic64_dec(atomic_t *v);                           /* v 减 1 */ 
int atomic64_sub_and_test(int i, atomic_t *v);            /* v 减 i, 然后判断 v 的值, 如果 v 的值为 0, 则返回非 0 值(true), 否则返回 0(false) */ 
int atomic64_inc_and_test(atomic_t *v);                   /* v 加 1, 然后判断 v 的值, 如果 v 的值为 0, 则返回非 0 值(true), 否则返回 0(false) */    
int atomic64_dec_and_test(atomic_t *v);                   /* v 减 i, 然后判断 v 的值, 如果 v 的值为 0, 则返回非 0 值(true), 否则返回 0(false) */ 
void atomic64_add_negative(int i, atomic_t *v);           /* v 加 i, 然后判断 v 的值, 如果 v 的值为负数, 则返回非 0 值(true), 否则返回 0(false) */ 
void atomic64_add_return(int i, atomic_t *v);             /* v 加 i 并返回结果*/ 
void atomic64_sub_return(int i, atomic_t *v);             /* v 减 i 并返回结果*/ 
void atomic64_inc_return(atomic_t *v);                    /* v 加 1 并返回结果*/ 
void atomic64_dec_return(atomic_t *v);                    /* v 减 1 并返回结果*/ 
void atomic64_add_unless(atomic_t *v, int a, int u);      /* 如果 v 的值不等于 u, 则 v 加 a, 并返回非 0 值(true), 否则 v 值不变, 并返回 0(false) */ 
void atomic64_inc_not_zero(atomic_t *v);                  /* 如果 v 的值不等于 0, 则 v 加 1, 并返回非 0 值(true), 否则 v 值不变, 并返回 0(false) */ 
```

#### 位原子操作
位原子操作主要针对 int, unsiged long, 或 void 的地址操作(因硬件平台而异)

位原子操作的宏在 asm/bitops.h 中定义
```c
void set_bit(int nr, void *addr);                            /* 将 addr 的第 nr 位设置为 1 */   
void clear_bit(int nr, void *addr);                          /* 将 addr 的第 nr 位设置为 0 */
void change_bit(int nr, void *addr);                         /* 如果 addr 的第 nr 位为 0, 则设为1; 否则设为 0 */
int test_and_set_bit(int nr, void *addr);                    /* 如果 addr 的 nr 位为 1, 则返回非 0 值(true), 否则将 addr 的 nr 位设置为 1, 并返回 0(false) */
int test_and_clear_bit(int nr, void *addr);                  /* 如果 addr 的 nr 位为 1, 则将 addr 的 nr 位设为 0, 并返回非 0 值(true), 否则返回 0(false) */
int test_and_change_bit(int nr, void *addr);                 /* 如果 addr 的 nr 位为 0, 则将 addr 的 nr 位设为 1, 并返回 0(false), 否则返回非 0 值(true) */
int test_bit(int nr, void *addr);                            /* /* 如果 addr 的 nr 位为 1, 则返回非 0 值(true), 否则返回 0(false) */
```
### Seqlock
顺序锁适用于小量、简单、经常读且很少被修改的资源

顺序锁的读锁和写锁并不互斥, 只是要求读锁释放的时候, 需要判断期间是否被获取或释放写锁

Seqlock 定义于 linux/seqlock.h
`linux/seqlock.h`
```c
typedef struct{
    unsiged sequence;
    spinlock_t lock;
} seqlock_t;
```

DEFINE_SEQLOCK 和 seqlock_init 宏用于初始化 seqlock_t
```c
DEFINE_SEQLOCK(lock);
void seqlock_init(seqlock_t *lock)
```
- DEFINE_SEQLOCK 定义并初始化 seqlock_t
- seqlock_init 只负责初始化

获取和释放写锁时, seqlock_t.sequence 都会被加1

获取读锁时, 序号获取当前 seqlock_t.sequence 值, 然后进入临界区读取资源; 读取完毕后, 必须先比较先前判断 seqlock_t.sequence 值是否已经改变(即写锁被获取或被释放), 如果有改变, 则需要重新进入临界区读取资源
```c
unsigned read_seqbegin(const seqlock_t *sl)
read_seqbegin_irqsave(sl,flags)
unsigned read_seqretry(const seqlock_t *sl, unsigned start)
read_seqretry_irqrestore(sl, start, flags)
```
- read_seqbegin 函数用于获取seqlock_t.sequence
- read_seqretry 函数用于判断seqlock_t.sequence是否发生变化
- read_seqbegin_irqsave 和 read_seqretry_irqrestore宏会记录当前cpu状态, 当 seqlock_t 被中断处理程序读取时使用

所以, 获取读锁的程序如下面模式编写
```c
unsiged int seq;
do{
    seq = read_seqbegin(&the_lock);
    /* 读临界区 */
}while read_seqretry(&the_lock, seq);
```

写锁和写锁之间是互斥的
```c
void write_seqlock(seqlock_t *lock);
int write_tryseqlock(seqlock_t *lock);
void write_sequnlock(seqlock_t *lock);

void write_seqlock_irqsave(seqlock_t *lock, unsigned long flags);
void write_seqlock_irq(seqlock_t *lock);
void write_seqlock_bh(seqlock_t *lock);


void write_sequnlock_irqrestore(seqlock_t *lock, unsigned long flags);
void write_sequnlock_irq(seqlock_t *lock);
void write_sequnlock_bh(seqlock_t *lock);
```
- write_seqlock 函数用于获取写锁, 如果 lock 指定的锁未被释放时会被阻塞
- write_tryseqlock 获取写锁, 成功获取返回非 0 值, 否则返回 0
- write_sequnlock 用于释放写锁

### 读取-复制-更新 (RCU)
锁机制实现并发访问数据存在两个问题
- 效率, 锁机制的实现需要对内存原子访问, 这种访问会降低cpu线性执行的效率; 另外, 在读写锁机制下, 写锁时排它锁, 无法实现写锁和读锁的并发操作, 会降低应用的执行效率
- 扩展性, 当 cpu 数量越来越多, 采取锁机制实现的并发访问数据的效率会越来越低

RCU机制提供了更高效的处理办法
- 读操作: 可以直接对访问共享资源(只要cpu支持内存原子访问), RCU 的读操作是不可抢占的
- 写操作: 对共享资源进行写操作前, 需要备份数据, 然后修改备份数据, 修改完毕后再用修改后的数据替换原数据, 但是替换旧数据的过程不是覆盖旧数据, 而是将旧数据的指针指向修改后的数据, 最后用于回收旧数据的后台进程会调用预先注册的函数来回收旧的数据空间

RCU机制的读和写并不互斥, 可同时进行

回收旧数据的后台进程会在所有读操作全部结束后回收旧的数据空间

如果读操作未全部结束, 写操作在修改备份数据后需要休眠等待, 直到读操作全部结束后才能更新旧数据

如果读操作被抢占或阻塞, 可能会导致系统死锁

是否允许由多个写操作并发访问数据取决于写操作之间的同步机制,

RCU不能代替读写锁, 因为如果写操作比较多时, 对读操作性能的提升不能弥补写操作同步机制带来的性能损失

#### RCU API
使用 RCU API 需要引入 linux/rcupdate.h

1. 读锁定
```c
static inline void rcu_read_lock(void);
static inline void rcu_read_lock_bh(void);
```
2. 读解锁
```c
static inline void rcu_read_unlock(void);
static inline void rcu_read_unlock_bh(void);
```

RCU 获取读锁标准代码如下
```c
rcu_read_lock();
// 读临界区代码
rcu_read_unlock();
```
rcu_read_lock 和 rcu_read_unlock 函数实际只是暂停和恢复内核抢占, 为防止在执行临界区代码时被其他进程打扰, rcu_read_lock_bh 和 rcu_read_unlock_bh 函数则在执行的时候暂停和恢复软中断


3. 注册回调函数
```c
struct rcu_head {
    struct rcu_head *next;
    void (*func) (struct rcu_head *head);
}
synchronize_rcu(void);
void call_rcu(struct rcu_head *head, void (*func) (struct rcu_head *head));
void call_rcu_bh(struct rcu_head *head, void (*func)(struct rcu_head *rcu));
```
call_rcu 函数由写执行单元调用, 它不会阻塞写执行单元, 因而可以在中断上下文或softirq使用, 这个函数将 func 挂载到 RCU 回调函数链上, 然后立即返回; 直到所有读执行单元已经执行完临界区代码( 每个执行单元执行完临界区代码后会发生一次上下文切换, 被称为经历一个 quiescent state, 所有读执行单元都经历一次 quiescent state 被称为经历一次 grace period ), 通过调用 func 来释放不在被使用的数据

synchronize_rcu 函数也可以由写执行单元调用，本质上它调用 call_rcu, 只不过它会阻塞写执行单元, 经历一次 grace period, 写执行单元才继续下一步操作。如果有多个写执行单元调用该函数, 他们将在一个grace period之后全部被唤醒, 至于哪一个写执行单元会被执行, 由它们采用的锁机制决定

call_rcu_bh 将 softinq 的完成也当做一次 quiescent state, 对应读执行单元调用 rcu_read_lock_bh 和 rcu_read_unlock_bh

4. 链表操作函数
```c
static inline void list_add_rcu(struct list_head *new, struct list_head *head);
static inline void list_add_tail_rcu(struct list_head *new, struct list_head *head);
static inline void list_del_rcu(struct list_head *entry);
static inline void list_replace_rcu(struct list_head *old, struct list_head *new);
list_for_each_rcu(pos, head);
list_for_each_safe_rcu(pos, n, head);
list_for_each_entry_rcu(pos, head, member);
list_for_each_continue_rcu(pos, head);
static inline void hlist_del_rcu(struct hlist_node *n);
static inline void hlist_add_head_rcu(struct hlist_node *n, struct hlist_head *h);
hlist_for_each_rcu(pos, head);
hlist_for_each_entry_rcu(tpos, pos, head, member);
```
- list_add_rcu 函数把链表项new插入到RCU保护的链表head的开头，使用内存栅保证了在引用这个新插入的链表项之前，新链表项的链接指针的修改对所有读操作是可见的。
- list_add_tail_rcu 函数将新的链表项new添加到被RCU保护的链表的末尾
- list_del_rcu 函数从RCU保护的链表中移除指定的链表项entry，并且把entry的prev指针设置为LIST_POISON2，但是并没有把entry的next指针设置为LIST_POISON1，因为该指针可能仍然在被读者用于遍历该链表。
- list_replace_rcu 函数使用新的链表项new取代旧的链表项old，内存栅保证在引用新的链表项之前，它的链接指针的修正对所有读操作可见。
- list_for_each_rcu 宏用于遍历由RCU保护的链表head，pos 为一个包含 struct list_head 结构的特定结构体,  只要在读端临界区使用该函数，它就可以安全地和其它_rcu链表操作函数（如list_add_rcu）并发运行。
- list_for_each_safe_rcu 宏类似于list_for_each_rcu，但不同之处在于它允许安全地删除当前链表项pos。
- list_for_each_entry_rcu 宏类似于list_for_each_rcu，不同之处在于它用于遍历指定类型的数据结构链表， pos 为一个包含 struct list_head 结构的特定结构体。
- list_for_each_continue_rcu 宏用于在退出点之后继续遍历由RCU保护的链表head。
- hlist_del_rcu 函数类似 list_del_rcu, 从哈希链表中移除节点 n
- hlist_add_head_rcu 函数类似 list_add_rcu，把链表项n插入到被RCU保护的哈希链表的开头，但同时允许读者对该哈希链表的遍历。内存栅确保在引用新链表项之前，它的指针修正对所有读操作可见。
- hlist_for_each_rcu 宏用于遍历由RCU保护的哈希链表head，只要在读端临界区使用该函数，它就可以安全地和其它_rcu哈希链表操作函数（如hlist_add_rcu）并发运行。
- hlist_for_each_entry_rcu 类似于hlist_for_each_rcu，不同之处在于它用于遍历指定类型的数据结构哈希链表，当前链表项pos为一包含struct list_head结构的特定的数据结构。

>备注: 可参考 http://www.ibm.com/developerworks/cn/linux/l-rcu/index.html