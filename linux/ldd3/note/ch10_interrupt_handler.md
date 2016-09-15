# 第十章 中断处理

中断是一种特殊的电子信号, 让外部设备用来引起CPU注意

驱动程序需要为设备的中断信号注册一个处理程序, linux 内核在 CPU 收到目标设备发出的中断信号时, 就会执行这个中断处理程序(ISR), 否则内核只会让 CPU 回复一个 ACK 信号给发出中断的设备, 然后忽略设备发出的中断信号

## 注册中断处理程序(ISR)

PC 系统通常只有十几条中断通道(IRQ), 因此 ISR 需要与特定的 IRQ建立对应关系

request_irq 函数用于在指定的 IRQ 上注册 ISR, 成功返回0, 失败返回负值(错误码)
```c
int request_irq(unsigned int irq, irqreturn_t (*handler)(int, void *, struct pt_regs *), unsigned long flags, const char *dev_name, void *dev_id);
```
- irq 是指定的中断通道编号
- handler 时被安装的 ISR 的函数指针
- flags 用于描述中断管理的选项, 取值如下：

    - SA_INTERRUPT: 注册一个快速ISR, 用于计时器中断, 以及 ISR 不可被打断的场景
    - SA_SHIRQ: 注册一个共享的ISR
    - SA_SAMPLE_RANDOM: 如果目标设备的中断信号是随机(非定期)发送的, 使用这个标志注册ISR
    
- dev_name 是 ISR 名, 会在 /proc/interrupts 中记录
- dev_id 指针用于共享 IRQ 和释放 IRQ; 如果想要独占 IRQ, 只需将 dev_id 设置为 NULL 即可, 也可指向一个特定的设备结构

free_irq 用于回收 IRQ
```c
void free_irq(unsigned int irq, void *dev_id);
```

### 自动检测 IRQ

#### 内核辅助检测法

Linux 内核提供一组函数 probe_irq_on 和 probe_irq_off(定义于linux/interrupt.h)用来检测 IRQ 编号, 仅适用于独占的中断
```c
unsigned long probe_irq_on(void);
int probe_irq_off(unsigned long);
```
- probe_irq_on 返回一个代表未占用的 IRQ 的掩码, 函数调用后, 才可以启动中断
- 调用 probe_irq_off 函数前, 必须关闭中断; 参数 long 是 probe_irq_on 返回的掩码; 如果没有发生中断, 函数返回0, 如果发生一次以上的中断, 返回一个负值

#### 自动检测法

原理: 逐一检测可能的"IRQ"值, 如果不缺东可能的"IRQ"值, 则需要从 IRQ0 测试到 IRQ NR_IRQS-1 (NR_IRQS 定义于 asm/irq.h)
```c
int trials[] =
        {
                3, 5, 7, 9, 0
        };
int tried[]  = {0, 0, 0, 0, 0};
int i, count = 0;

for (i = 0; trials[i]; i++)
        tried[i] = request_irq(trials[i], short_probing,
                               SA_INTERRUPT, "short probe", NULL);

do
{
        short_irq = 0; /* none got, yet */
        outb_p(0x10,short_base+2); /* enable */
        outb_p(0x00,short_base);
        outb_p(0xFF,short_base); /* toggle the bit */
        outb_p(0x00,short_base+2); /* disable */
        udelay(5); /* give it some time */

        /* the value has been set by the handler */
        if (short_irq == 0) { /* none of them? */

                printk(KERN_INFO "short: no irq reported by probe\n");
        }
        /*
        * If more than one line has been activated, the result is
        * negative. We should service the interrupt (but the lpt port
        * doesn't need it) and loop over again. Do it at most 5 times
        */
} while (condition);

/* end of loop, uninstall the handler */
for (i = 0; trials[i]; i++)
        if (tried[i] == 0)
                free_irq(trials[i], NULL);

if (short_irq < 0)
        printk("short: probe failed %i times, giving up\n", count);

```

## ISR 函数

ISR 函数负责接收、反馈发出中断的设备, 依据中断的意义来读写数据

ISR 函数遵循的限制:

- 不能与用户空间传输数据
- 不能做任何导致休眠的动作
- 只能以 GFP_ATOMIC 的形式分配内存
- 不能锁定信号量
- 不能调用 schedule 函数

ISR 函数原型
```c
irqreturn_t (*handler)(int irq, void *dev_id, struct pt_regs *regs)
```
- irq 是 IRQ 号码
- dev_id 是用户端数据, 当中断发生时, request_irq 函数传入的 dev_id 参数就会交给 ISR 函数， 可用于传递需要在中断时期针对个别设备处理的数据结构
- regs 用于存储CPU进入中断之前的状态(寄存器中的值), 只用于debug或监控系统状态

返回值:

- 返回 IRQ_HANDLED 表示ISR发现设备发出了需要注意的中断
- 返回 IRQ_NONE 表示中断被忽略
- IRQ_RETVAL(handled) 宏用于产生返回值, handled 为非零值代表 ISR 能够处理中断

### 中断的生效与失效

使用自旋锁时, 必须让中断暂时失效, 以避免造成锁死

#### 禁用指定线路上的中断

这种方法一般很少使用, 因为现在的系统上大多数的中断都是共享的，共享的中断线路不能被压制

disable_irq 和 enable_irq(定义于 asm/irq.h)需要成对调用, 对同一个 IRQ 连续调用两次 disable_irq , 则必须调用两次 enable_irq
```c
void disable_irq(int irq);
void disable_irq_nosync(int irq);
void enable_irq(int irq);
```
- disable_irq 会等待IRQ上当时正在执行的ISR结束之后才会返回
- disable_irq_nosync 在改变了外设上的 PIC(可编程中断控制器, 这三个函数的操作对象, 用于控制IRQ上的信号能否到达CPU寄存器) 寄存器的bit掩码之后立即返回, 不会影响IRQ上当时正在执行的ISR

#### 禁用所有线路上的中断

local_irq_save 和 local_irq_disable 函数(定义于asm/system.h)用来禁用所有线路上的中断
```c
void local_irq_save(unsigned long flags);
void local_irq_disable(void);
```
- local_irq_save 会将当时中断状态存入 flags, 然后才禁用中断
- local_irq_disable 直接关闭中断, 不会保存状态

local_irq_restore 和 local_irq_enable 用来恢复中断, 
```c
void local_irq_restore(unsigned long flags); 
void local_irq_enable(void);
```
- local_irq_restore 与 local_irq_save 成对使用
- local_irq_enable 与 local_irq_disable 成对使用

## 中断的低半部和高半部

Linux 内核将 ISR 分为高半部和低半部, 高半部是指用 request_irq 注册的那个函数, 用于在中断发生时执行需处理的例行任务; 低半部是将高半部交给调度器的一个函数, 由调度器另外找安全的时间点才开始执行

高半部和低半部的差别:

- 低半部的执行期间允许发生中断
- 高半部只做两件工作: 将设备上的新数据存储到缓冲区, 然后调度低半部
- 低半部会唤醒正在等待数据的进程

### 通过 Tasklet 机制执行低半部函数

Tasklet 机制是执行低半部函数的首选, 适用于快速执行的低半部函数, 但所有操作必须在原子环境下

### 通过工作队列机制执行低半部函数

工作队列机制可用于执行允许延迟的低半部函数, 如果函数中有可能触发休眠的操作, 可以选择使用工作队列

## 中断共享

现代硬件的架构设计允许多个设备共同IRQ(比如PCI设备), 因此 Linux 内核让所有总线(包括ISA总线)都可以共享中断, 只要目标设备支持中断共享的操作模式, 驱动程序就可以设计称中断共享的形式

中断共享式 ISR 与普通 ISR 的区别:

- 使用 request_irq 函数注册 ISR 时, flags 参数必须设置为 SA_SHIRQ, dev_id 参数必须是系统上独一无二的数值(可以是高半部函数返回的irqreturn_t结构体指针)
- 高半部处理函数必须能区分接收到的中断是否由注册该中断的设备发出, 这继续要硬件支持, 也需要在驱动程序中有相关的处理逻辑(一般使用 dev_id 参数来判断)

大多数支持中断共享的硬件, 都能提供它正在使用的IRQ号码, 所以没必要自动检测IRQ号码

和普通ISR一样, 中断共享式ISR也使用free_irq 函数回收 IRQ

共享式 ISR 不能使用 enable_irq 和 disable_irq 函数禁用/重启中断

## 中断驱动式 I/O

目前工人最好的缓冲机制就是中断驱动式I/O:在中断期间由ISR填充输入缓冲区; 在进程读取设备时清理缓冲区; 输出缓冲区由进程填充, 在中断期间清理到设备上

要让中断驱动的数据传输能够成功, 硬件应该要能以一下条件产生中断信号:

- 对于输入(从外部设备到CPU), 设备必须在新数据到达, 而且可供系统处理器获取的情况下, 对CPU发出中断信号
- 对于输出(从CPU到外部设备), 设备必须在准备好接收新数据, 或报告已成功传输出前次的数据时, 向CPU发出中断信号