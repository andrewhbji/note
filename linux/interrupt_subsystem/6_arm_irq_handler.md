# ARM 中断处理过程

## 中断处理的准备过程

ARM 处理器有多种 processor mode，例如 user mode（用户空间的 AP 所运行的模式）、supervisor mode（即 SVC mode，大部分的内核态代码运行的模式）、IRQ mode（发生中断后，处理器会切换到该模式）等。

Linux 内核在中断处理过程中，ARM 处理器大部分都是处于SVC mode。但是，实际上产生中断的时候，ARM 处理器实际上先进行一小段 IRQ mode 操作，之后会进入 SVC mode 进行真正的 IRQ 异常处理。

>备注：除了 IRQ mode，linux 内核在处理 ABT mode（当发生 data abort exception 或者 prefetch abort exception 的时候进入的模式）和 UND mode（处理器遇到一个未定义的指令的时候进入的异常模式）的时候也是采用了相同的策略。

### 中断模式的 stack 准备

由于 IRQ/ABT/UND mode 只是一个过度，在 IRQ/ABT/UND mode 和 SVC mode 之间总是需要一个 stack 保存数据，这就是中断模式的 stack，定义如下：
```c
struct stack {
    u32 irq[3];
    u32 abt[3];
    u32 und[3];
} ____cacheline_aligned;


static struct stack stacks[NR_CPUS]; 
```

CPU 初始化时，cpu_init 函数会进行中断模式 stack 的设置
```c
void notrace cpu_init(void)
{

    unsigned int cpu = smp_processor_id();
    struct stack *stk = &stacks[cpu];   /* 获取 CPU 对于的 irq/abt/und mode 的 stack 指针 */

    ……

    #ifdef CONFIG_THUMB2_KERNEL
    #define PLC    "r"                  /* Thumb-2 下，msr 指令不允许使用立即数，只能使用寄存器 */
    #else
    #define PLC    "I"
    #endif


    __asm__ (
    "msr    cpsr_c, %1\n\t"             /* 让 CPU 进入 IRQ mode */
    "add    r14, %0, %2\n\t"            /* r14 寄存器保存 stk->irq */
    "mov    sp, r14\n\t"                /* 设定 IRQ mode 的 stack 为 stk->irq */
    "msr    cpsr_c, %3\n\t"
    "add    r14, %0, %4\n\t"
    "mov    sp, r14\n\t"                /* 设定 abt mode 的 stack 为 stk->abt */
    "msr    cpsr_c, %5\n\t"
    "add    r14, %0, %6\n\t"
    "mov    sp, r14\n\t"                /* 设定 und mode 的 stack 为 stk->und */
    "msr    cpsr_c, %7"                 /* 回到 SVC */
        :                               /* 上面是 code，下面的 output 部分是空的 */
        : "r" (stk),                    /* 对应上面代码中的 %0 */
          PLC (PSR_F_BIT | PSR_I_BIT | IRQ_MODE),   /* 对应上面代码中的 %1 */
          "I" (offsetof(struct stack, irq[0])),     /* 对应上面代码中的 %2 */
          PLC (PSR_F_BIT | PSR_I_BIT | ABT_MODE),   /* 对应上面代码中的 %3 */
          "I" (offsetof(struct stack, abt[0])),     /* 对应上面代码中的 %4 */
          PLC (PSR_F_BIT | PSR_I_BIT | UND_MODE),   /* 对应上面代码中的 %5 */
          "I" (offsetof(struct stack, und[0])),     /* 对应上面代码中的 %6 */
          PLC (PSR_F_BIT | PSR_I_BIT | SVC_MODE)    /* 对应上面代码中的 %7 */
        : "r14");                                   /* 上面是 input 操作数列表，r14 是要 clobbered register 列表 */
} 
```

>备注：电源管理模块初始化时，也需要作类似的设置，因为电源管理状态在进入 sleep 的时候，CPU 需要丢失 irq、abt 和 und stack point 寄存器的值，在 CPU resume 的过程中，要调用 cpu_init 来重新设定这些值

### SVC 模式的 stack 准备

和用户空间一样，内核空间的代码也需要保存局部变量的栈。这个内核栈就是处于 SVC mode 时候使用的栈。

当进程切换的时候，整个硬件和软件的上下文都会进行切换，包括 SVC mode 的sp寄存器的值被切换到调度算法选定的新的进程的内核栈.

Linux 最开始启动时，系统只有 idle 进程，该进程只有一个 kernel 线程。idle 进程的内核栈是静态定义的：
```c
union thread_union init_thread_union __init_task_data =
    { INIT_THREAD_INFO(init_task) };

union thread_union {
    struct thread_info thread_info;
    unsigned long stack[THREAD_SIZE/sizeof(long)];  /* ARM平台 THREAD_SIZE 是 8192 byte，占两个 page frame */
}; 
```

Linux kernel 在创建进程（包括用户进程和内核线程）的时候都会分配一个（或者两个，和配置相关）page frame，具体代码如下：
```c
static struct task_struct *dup_task_struct(struct task_struct *orig)
{
    ......

    ti = alloc_thread_info_node(tsk, node);
    if (!ti)
        goto free_tsk;

    ......
} 
```

### 异常向量表的准备

ARM 处理器处理异常（如处理中断）时，处理器会暂停当前指令的执行，保存现场，转而去执行对应的异常向量处的指令，处理完该异常时，恢复现场，回到原来的那点去继续执行程序。

ARM Linux 异常向量表定义如下：
```
	.section .vectors, "ax", %progbits
__vectors_start:
	W(b)	vector_rst
	W(b)	vector_und
	W(ldr)	pc, __vectors_start + 0x1000
	W(b)	vector_pabt
	W(b)	vector_dabt
	W(b)	vector_addrexcptn
	W(b)	vector_irq                      /* IRQ Vector */
	W(b)	vector_fiq
```

ARM Linux 默认会通过MMU将异常向量表映射在高虚拟地址（0xffff0000），被称为 high vector，代码如下：
```c
tatic void __init devicemaps_init(const struct machine_desc *mdesc)
{
    ……
    vectors = early_alloc(PAGE_SIZE * 2);   /* 分配两个page的物理页帧 */

    early_trap_init(vectors);               /* 初始化并填充异常向量表和相关 help function  */

    ……
    map.pfn = __phys_to_pfn(virt_to_phys(vectors));
    map.virtual = 0xffff0000;
    map.length = PAGE_SIZE;
#ifdef CONFIG_KUSER_HELPERS
    map.type = MT_HIGH_VECTORS;
#else
    map.type = MT_LOW_VECTORS;
#endif
    create_mapping(&map);               /* 映射0xffff0000的那个page frame */

    if (!vectors_high()) {              /* 如果SCTLR.V的值设定为low vectors，那么还要映射0地址开始的memory*/
        map.virtual = 0;
        map.length = PAGE_SIZE * 2;
        map.type = MT_LOW_VECTORS;
        create_mapping(&map);
    }

    ……
    map.pfn += 1;
    map.virtual = 0xffff0000 + PAGE_SIZE;
    map.length = PAGE_SIZE;
    map.type = MT_LOW_VECTORS;
    create_mapping(&map);               /* 映射high vecotr开始的第二个page frame */

    ……
} 
```
- 这里共分配了两个 page frame，vectors table 和 kuser helper函数（内核空间提供的函数，但是用户空间使用）占用了一个 page frame，异常处理的stub函数占用了另外一个 page frame。
>备注：Linux 系统中，0--3G 的空间是用户空间（低地址）

```c
void __init early_trap_init(void *vectors_base)
{
    unsigned long vectors = (unsigned long)vectors_base;
    extern char __stubs_start[], __stubs_end[];
    extern char __vectors_start[], __vectors_end[];
    unsigned i;

    vectors_page = vectors_base;

    /* 将整个 vector table 那个 page frame 填充成未定义的指令。起始 vector table 加上 kuser helper 函数并不能完全的充满这个 page，有些缝隙。如果不这么处理，当极端情况下（程序错误或者HW的issue），CPU 可能从这些缝隙中取指执行，从而导致不可知的后果。如果将这些缝隙填充未定义指令，那么CPU可以捕获这种异常。*/
    for (i = 0; i < PAGE_SIZE / sizeof(u32); i++)
        ((u32 *)vectors_base)[i] = 0xe7fddef1;

    /* 拷贝vector table，拷贝 stub function */
    memcpy((void *)vectors, __vectors_start, __vectors_end - __vectors_start);
    memcpy((void *)vectors + 0x1000, __stubs_start, __stubs_end - __stubs_start);

    kuser_init(vectors_base); /* copy kuser helper function */

    flush_icache_range(vectors, vectors + PAGE_SIZE * 2);
    modify_domain(DOMAIN_USER, DOMAIN_CLIENT);

} 
```

## ARM HW 对中断事件的处理

CPU 的全局中断打开后，就可以处理来自外设的中断事件。

当外设发生中断，会改变 IRQ line 电平或者上升/下降沿电信号，中断控制器检测到了 IRQ line 的电信号，会将该中断信号通知一个 CPU，CPU 感知到 中断时间后，就会执行一系列动作：

1. 修改 CPSR（Current Program Status Register）寄存器中的M[4:0]，将 CPU 切换到 IRQ mode。

    M[4:0]表示了 ARM 处理器当前处于的模式, ARM 定义了9种处理模式，包括:
    
| 处理器模式 | 缩写 | 对应的M[4:0]编码 | Privilege level |
|:--|:--|:--|:--|
| User  | usr  | 10000  | PL0  |
| FIQ  |  fiq |  10001 |  PL1 |
| IRQ  | irq  | 10010  | PL1  |
| Supervisor  | svc  | 10011  | PL1  |
| Monitor  | mon  | 10110  | PL1  |
| Abort  | abt  | 10111  | PL1  |
| Hyp  | hyp  | 11010  | PL2  |
| Undefined  | und  | 11011  | PL1  |
| System  | sys  | 11111  | PL1  |


2. 保存发生中断那一时刻的 CPSR 值（step 1 之前的状态）和 PC 值。

    每种处理模式下，可使用的 ARM core register（R0～R15）都不同，其中 PC 和 CPSR 寄存器是共用的，详见下表

| 寄存器/模式 | Usr | System | Hyp | Supervisor | abort | undefined | Monitor | IRQ | FIQ |
|:--|:--|:--|:--|:--|:--|:--|:--|:--|:--|
|  R0  | R0_usr |  |  |  |  |  |  |  |  |
|  R1  | R1_usr |  |  |  |  |  |  |  |  |
|  R2  | R2_usr |  |  |  |  |  |  |  |  |
|  R3  | R3_usr |  |  |  |  |  |  |  |  |
|  R4  | R4_usr |  |  |  |  |  |  |  |  |
|  R5  | R5_usr |  |  |  |  |  |  |  |  |
|  R6  | R6_usr |  |  |  |  |  |  |  |  |
|  R7  | R7_usr |  |  |  |  |  |  |  |  |
|  R8  | R8_usr |  |  |  |  |  |  |  | R8_fiq|
|  R9  | R9_usr |  |  |  |  |  |  |  | R9_fiq |
|  R10  | R10_usr |  |  |  |  |  |  |  |  R10_fiq |
|  R11  | R11_usr |  |  |  |  |  |  |  |  R11_fiq |
|  R12  | R12_usr |  |  |  |  |  |  |  |  R12_fiq |
|  SP  | SP_usr |  | SP_hyp | SP_svc |  SP_abt |  SP_und |  SP_mon |  SP_irq |  SP_fiq |
|  LR  | LR_usr |  |  | LR_svc | LR_abt | LR_und | LR_mon | LR_irq | LR_fiq |
|  PC  |  |  |  |  |  |  |  |  |  |
| CPSR |  |  |  |  |  |  |  |  |  |
|  |  |  |  | SPSR_svc | SPSR_abt | SPSR_und | SPSR_mon | SPSR_irq | SPSR_fiq |
|  |  |  | ELR_hyp |  |  |  |  |  |  |

ARM CPU 切换到 IRQ mode 时，会将中断发生那一刻的 CPSR 值保存在 SPSR_irq 寄存器中

PC 寄存器后续会被修改为 irq exception vector，因此 保存 PC 值本质上就是保存发生中断那一时刻所执行指令的下一条指令的地址，具体的返回地址保存在 LR_irq 寄存器中。

ARM CPU，LR_irq = PC - 4

>备注： LR_irq = PC - 4 的原因是：
> 由于ARM采用流水线结构，当CPU正在执行某一条指令的时候，其实取指的动作已经执行，这时候 PC 值 ＝ 正在执行的指令地址 ＋ 8，此时执行现场如下所示：
```
－－－－> 发生中断的指令 
         发生中断的指令＋4
－PC－－> 发生中断的指令＋8
         发生中断的指令＋12
```
> 发生中断时，当前正在执行的指令要执行完毕，但是已经完成取指、译码的指令则终止执行。当正在执行的指令执行完毕后，PC 值会继续加 4（指向发生中断的指令＋12），因此中断发生后，ARM core 的硬件现场如下
```
－－－－> 发生中断的指令
         发生中断的指令＋4 <-------中断返回的指令是这条指令
         发生中断的指令＋8
－PC－－> 发生中断的指令＋12 
```
> 这时候的 PC 值比发生中断时候的指令超前 12，减去 4 之后，lr_irq 中保存了（发生中断的指令＋8）的地址。而实际上从中断返回的指令应是”发生中断的指令＋4“，这需要后续交给软件处理。
> 为什么 HW 不直接进行 -8 处理，原因是 ARM 的异常处理的硬件逻辑不仅仅处理 IRQ 异常，还处理其他异常，而不同的异常起放的返回地址并不统一，所以硬件只能硬件只是 -4

3. mask IRQ exception，也就是设定 CPSR.I = 1 

4. 设定 PC 寄存器为 IRQ exception vector。此时 ARM 处理器就会跳转到 IRQ 的 exception vector 地址，后续的动作就是软件行为。

## 软件对 ARM 中断事件的处理

### IRQ mode 中的处理

IRQ mode 的处理都在 vector_irq 中

vector_irq 定义了一个 lookup table，并通过 vector_stub 宏跳转到 lookup table 中定义的目标向量
```
vector_stub    irq, IRQ_MODE, 4     @ 减去4，确保返回发生中断之后的那条指令

.long    __irq_usr            @  0  (USR_26 / USR_32)   <---> base address + 0
.long    __irq_invalid            @  1  (FIQ_26 / FIQ_32)
.long    __irq_invalid            @  2  (IRQ_26 / IRQ_32)
.long    __irq_svc            @  3  (SVC_26 / SVC_32)   <---> base address + 12
.long    __irq_invalid            @  4
.long    __irq_invalid            @  5
.long    __irq_invalid            @  6
.long    __irq_invalid            @  7
.long    __irq_invalid            @  8
.long    __irq_invalid            @  9
.long    __irq_invalid            @  a
.long    __irq_invalid            @  b
.long    __irq_invalid            @  c
.long    __irq_invalid            @  d
.long    __irq_invalid            @  e
.long    __irq_invalid            @  f 
```

vector_stub 宏代码如下：
```
.macro    vector_stub, name, mode, correction=0
    .align    5

vector_\name:
    .if \correction
    sub    lr, lr, #\correction                         @ LR_irq - correction 获得发生异常时的 PC 值， vector_irq 的 correction = 4
    .endif

    @
    @ Save r0, lr_ (parent PC) and spsr_                @ sp_irq 被初始化为12个字节，依次保存了中断发生时的 r0 值、PC 值以及 CPSR值
    @ (parent CPSR)
    @
    stmia    sp, {r0, lr}        @ save r0, lr          @ 保存中断发生时的 r0、PC 值到 sp_irq 栈
    mrs    lr, spsr                                     @ 将中断发生时的 CPSR 值保存在 lr 寄存器
    str    lr, [sp, #8]        @ save spsr              @ 保存中断发生时的 CPSR 值到 sp_irq 栈

    @
    @ Prepare for SVC32 mode.  IRQs remain disabled.
    @
    mrs    r0, cpsr                                     @ 在切换到 SVC mode 前，做些准备：修改 SPSR 的值为 SVC_MODE
    eor    r0, r0, #(\mode ^ SVC_MODE | PSR_ISETSTATE)
    msr    spsr_cxsf, r0

    @
    @ the branch table must immediately follow this code
    @
    and    lr, lr, #0x0f                    @ lr 保存了发生 IRQ 时候的 CPSR，通过 and 操作，可以获取 CPSR.M[3:0] 的值，这时候，如果中断发生在用户空间，lr=0，如果是内核空间，lr=3 
THUMB( adr    r0, 1f            )           @ 根据当前 PC 值，获取 lable 1 的地址 
THUMB( ldr    lr, [r0, lr, lsl #2]  )       @ 根据当前 mode 将 lr 的值设置为 __irq_usr 的地址或者 __irq_svc 的地址 
    mov    r0, sp                           @ 将 irq mode 的 sp_irq 栈指针通过 r0 传递给即将跳转的函数 
ARM(    ldr    lr, [pc, lr, lsl #2]    )    @ 根据 mode 将 lr 设置为 __irq_usr 或者 __irq_svc
    movs    pc, lr                          @ 把 lr 的值付给 pc，并隐性的将 SPSR 值复制到 CPSR，实现切换到 SVC mode
ENDPROC(vector_\name)

    .align    2
    @ handler addresses follow this label
1:
    .endm 
```

### 代码运行在用户空间时发生中断

代码运行在用户空间时发生中断，vector_stub 跳转到 __irq_usr 向量：
```
    .align    5
__irq_usr:
    usr_entry                   @ 执行 usr_entry 宏
    kuser_cmpxchg_check
    irq_handler                 @ 执行 irq_handler 宏
    get_thread_info tsk         @ tsk 是 r9，指向当前的 thread info数据结构 
    mov    why, #0              @ why是r8
    b    ret_to_user_from_irq   @ 返回中断发生时的现场
```

usr_entry 宏会保存中断时候的现场，即发生中断那一刻的硬件上下文（各个寄存器）保存在了 SVC mode 栈上的值
```
.macro    usr_entry
    sub    sp, sp, #S_FRAME_SIZE        @ 设置 sp_svc 栈的大小为18个寄存器，栈顶为 r0
    stmib    sp, {r1 - r12}             @ r1--r12 入 sp_svc 栈，stmib指令 压入 r1 时会预留 r0 的位置，因为没有”！“修饰符，完成压栈后不会更新 sp_svc 的值

    ldmia    r0, {r3 - r5}              @ r0 指向 irq stack，此时 r3 是中断现场的 r0 值，r4 是中断现场的 PC 值，r5 是中断现场的 CPSR 值，此时
    add    r0, sp, #S_PC                @ 把 r0 赋值为 S_PC 的值，使 r0 指向 ARM_pc 寄存器在 svc mode 栈上的位置
    mov    r6, #-1                      @ orig_r0 的值

    str    r3, [sp]                     @ 保存中断时的 r0


    stmia    r0, {r4 - r6}              @ 将发生中断 PC，CPSR 和 orig r0 压栈，实际上这段操作就是将 irq 栈的中断现场搬移到内核栈
    stmdb    r0, {sp, lr}^              @ 将发生中断时 sp 和 lr 压入栈中剩余两个位置
    .endm 
```

irq_handler 宏有两种配置，ARM 使用的是配置 CONFIG_MULTI_IRQ_HANDLER 选项，这种情况下，linux kernel 允许 run time 设定 irq handler。如果需要让一个 linux kernel image 支持多个平台，就需要配置这个选项；另外一种是传统的 linux 的做法，irq_handler 实际上就是 arch_irq_handler_default，这种方法在内核中用的越来越少
```
    .macro    irq_handler
#ifdef CONFIG_MULTI_IRQ_HANDLER
    ldr    r1, =handle_arch_irq
    mov    r0, sp                   @ 设定传递给 machine 定义的 handle_arch_irq 的参数
    adr    lr, BSYM(9997f)          @ 设定返回地址
    ldr    pc, [r1]
#else
    arch_irq_handler_default
#endif
9997:
    .endm
```
```
.macro    arch_irq_handler_default
    get_irqnr_preamble r6, lr               @ get_irqnr_preamble 为中断处理做准备，有些平台根本不需要这个步骤，直接定义为空
1:    get_irqnr_and_base r0, r2, r6, lr     @ r0 保存了本次解析的 irq number，r2 是 irq 状态寄存器的值，r6 是 irq controller 的 base address，lr 是 scratch register
    movne    r1, sp
    @
    @ asm_do_IRQ 需要两个参数，一个是 irq number（保存在r0）
    @                                          另一个是 struct pt_regs *（保存在r1中）
    adrne    lr, BSYM(1b)           @ 返回地址设定为符号1，也就是说要不断的解析 irq 状态寄存器的内容，得到 IRQ number，直到所有的 irq number 处理完毕
    bne    asm_do_IRQ 
    .endm 
```

### 代码运行在内核空间时发生中断

代码运行在内核空间时发生中断，vector_stub 跳转到 __irq_svc 向量：
```
.align    5
__irq_svc:
    svc_entry           @ 保存发生中断那一刻的现场保存在内核栈上
    irq_handler         @ 具体的中断处理，同 user mode 的处理。

#ifdef CONFIG_PREEMPT   @ 和 preempt 相关的处理
    get_thread_info tsk
    ldr    r8, [tsk, #TI_PREEMPT]        @ get preempt count
    ldr    r0, [tsk, #TI_FLAGS]        @ get flags
    teq    r8, #0                @ if preempt count != 0
    movne    r0, #0                @ force flags to 0
    tst    r0, #_TIF_NEED_RESCHED
    blne    svc_preempt
#endif

    svc_exit r5, irq = 1            @ 返回中断发生时的现场
```

```
    .macro    svc_entry, stack_hole=0
sub    sp, sp, #(S_FRAME_SIZE + \stack_hole - 4)    @ sp指向struct pt_regs中r1的位置
stmia    sp, {r1 - r12}                             @ 寄存器入栈。

ldmia    r0, {r3 - r5}
add    r7, sp, #S_SP - 4                            @ r7指向struct pt_regs中r12的位置
mov    r6, #-1                                      @ orig r0设为-1
add    r2, sp, #(S_FRAME_SIZE + \stack_hole - 4)    @ r2是发现中断那一刻stack的现场
str    r3, [sp, #-4]!                               @ 保存r0，注意有一个！，sp会加上4，这时候sp就指向栈顶的r0位置了

mov    r3, lr                                       @ 保存svc mode的lr到r3
stmia    r7, {r2 - r6}                              @ 压栈，在栈上形成形成struct pt_regs
.endm 
```

## 中断退出过程

无论是内核空间时发生中断还是用户空间时发生中断，中断处理完成后，都要返回中断发生时的现场

#### 中断发生在 user mode 下的退出过程，代码如下：
```
ENTRY(ret_to_user_from_irq)
    ldr    r1, [tsk, #TI_FLAGS]
    tst    r1, #_TIF_WORK_MASK      @ 如果设置 _TIF_NEED_RESCHED  _TIF_SIGPENDING _TIF_NOTIFY_RESUME 这三个 flag 其中之一，就会进入 work_pending，否则进入 no_work_pending
    bne    work_pending
no_work_pending:
    asm_trace_hardirqs_on           @ 和 irq flag trace 相关

    /* perform architecture specific actions before user return */
    arch_ret_to_user r1, lr         @ 有些硬件平台需要在中断返回用户空间做一些特别处理
    ct_user_enter save = 0          @ 和 trace context 相关

    restore_user_regs fast = 0, offset = 0      @ 将中断发生时的现场恢复到 ARM 的各个寄存器中
ENDPROC(ret_to_user_from_irq)
ENDPROC(ret_to_user) 
```

```
    .macro    restore_user_regs, fast = 0, offset = 0
ldr    r1, [sp, #\offset + S_PSR]       @ r1 保存了 pt_regs 中的 spsr，也就是发生中断时的 CPSR
ldr    lr, [sp, #\offset + S_PC]!       @ lr 保存了 PC 值，同时 sp 移动到了 pt_regs 中 PC 的位置
msr    spsr_cxsf, r1                    @ 赋值给 spsr，进行返回用户空间的准备
clrex                                   @ clear the exclusive monitor

.if    \fast
ldmdb    sp, {r1 - lr}^                 @ get calling r1 - lr
.else
ldmdb    sp, {r0 - lr}^                 @ 将保存在内核栈上的数据迁移到用户态的 r0～r14 寄存器
.endif
mov    r0, r0                           @ NOP 操作，ARMv5T 之前的需要这个操作
add    sp, sp, #S_FRAME_SIZE - S_PC     @ 现场已经恢复，移动 svc mode 的 sp 到原来的位置
movs    pc, lr                          @ 返回用户空间
.endm 
```

#### 中断发生在 svc mode 下的退出过程。具体代码如下： 

```
    .macro    svc_exit, rpsr, irq = 0
.if    \irq != 0
    @ IRQs already off
.else
    @ IRQs off again before pulling preserved data off the stack
disable_irq_notrace
.endif
msr    spsr_cxsf, \rpsr     @ 将中断现场的 cpsr 值保存到 spsr 中，准备返回中断发生的现场

ldmia    sp, {r0 - pc}^     @ 这条指令是 ldm 异常返回指令，这条指令除了字面上的操作，还包括了将 spsr copy 到 cpsr 中。

.endm 
```