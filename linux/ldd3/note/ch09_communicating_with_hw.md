# 第九章 与硬件通信

## I/O 端口与 I/O 内存

## 使用 I/O 端口

## I/O 端口示例

## 使用 I/O 内存

## I/O 端口与 I/O 内存
CPU 一般通过读写外部设备的寄存器控制外部设备, 这些寄存器可以在一段连续的地址空间范围, 这些地址空间可能在内存地址空间, 也可能位于I/O地址空间

从硬件的角度看, 内存地址空间与I/O地址空间没有差别:
- 要求读写操作的指令都是通过控制总线上的读写电子信号来传递给内存或外部设备
- 数据的地址是通过地址总线来传递
- 数据本身通过数据总线来传递

x86 系列处理器架构让外部设备和物理内存分别拥有各自专属的地址总线, 让I/O端口有专属的读写电子信号线, 并提供特殊的CPU指令来访问I/O端口; 在访问周边外设时, 通常是读写I/O端口

所以Linux 内核虚拟了一组 I/O 端口访问机制, 并假设Linux支持的每一种硬件平台都有I/O端口

### I/O 寄存器与内存

硬件寄存器与系统内存的性质相似, 但程序访问I/O端口时, 需要特别小心, 不能将I/O端口当做一般的内存, 因为编译器的优化计数会影响实际的访问顺序, 可能导致改变了预期的I/O行为

I/O缓存器与RAM之间的主要差异, 在于I/O操作有副作用:
- 写入内存时, 唯一的作用就是改变目标位置的内容值; 读区内存时, 则是返回上一次写在目标位置的内容值
- 访问I/O寄存器时, 却可能通过读取或写入操作改变设备的状态

对于硬件高速缓存的问题, 只要实现设定好底层硬件, 要求禁用I/O区的任何硬件高速缓存功能即可

要避免编译器将访问I/O寄存器的程序优化, Linux 提供的办法是在一般指令(可被优化或改变顺序)与特殊指令(必须以原貌作用在硬件上)之间设置一个内存格栅, 让编译器知道哪些程序段不可被优化

barrier 函数(定义于 linux/kernel.h)可以要求编译器插入一个内存格栅
```c
void barrier(void)
```

rmb wmb mb read_barrier_depends 函数(定义于 asm/system.h)将硬件内存格栅安排在编译出来的指令流程中
```c
void rmb(void);
void read_barrier_depends(void);
void wmb(void);
void mb(void);
void smp_rmb(void);
void smp_read_barrier_depends(void);
void smp_wmb(void);
void smp_mb(void);
```
- rmb 函数保证任何出现在格栅之前的读取操作, 都会在后续的任何读取操作之前完成
- wmb 函数保证写入操作被依序执行
- mb 函数同时保证读写操作被依序执行
- read_barrier_depends 是 rmb 的弱化版本, 而 read_barrier_depends 允许互不相干的读取指令可被改变顺序, 一般使用 rmb, 除非认为使用 read_barrier_depends 更为高效
- smp* 函数用于SMP系统

内存格栅在驱动程序中的典型用法:
```c
writel(dev->registers.addr, io_destination_address);
writel(dev->registers.size, io_size);
writel(dev->registers.operation, DEV_READ);
wmb();
writel(dev->registers.control, DEV_GO);
```
在本例中, 内存格栅会确保前三个 writel 操作都完成后才执行最后一个 writel 操作

在x86 平台, cpu 一般不会改变写入操作的执行顺序, 但读取操作的执行顺序可能会被改变, 所以 mb 函数相较 wmb 比较低效

某些平台允许"一次赋值操作"与"一个内存格栅"共同组成一个较高效的执行组合, linux 内核提供了一些宏(定义于 asm/system.h)支持这样的组合
```c
#define set_mb(var, value) do {var = value; mb();}  while 0
#define set_wmb(var, value) do {var = value; wmb();} while 0
#define set_rmb(var, value) do {var = value; rmb();} while 0
```

## 使用 I/O 端口

### 分配 I/O 端口

使用I/O端口前需要分配

request_region 函数用于分配I/O端口的地址范围
```c
#include <linux/ioport.h>
struct resource *request_region(unsigned long first, unsigned long n, const char *name);
```
- 如果 first 和 n 指定的I/O端口地址范围已经被分配, 则返回 NULL(/proc/ioports 文档记录了所有已分配的I/O地址范围)

release_region 函数用于释放I/O地址
```c
void release_region(unsigned long start, unsigned long n); 
```

check_region 用于检查I/O端口地址范围是否已经被占用, 若占用则返回负值
```c
int check_region(unsigned long first, unsigned long n); 
```

### 操作 I/O 端口
I/O 端口带宽一般分为 8-bits 16-bits 32-bits

Linux 内核提供操作 I/O 端口的函数(定义于 asm/io.h)

inb outb 函数用于读或写 8-bits 带宽 端口
```c
unsigned inb(unsigned port);
void outb(unsigned char byte, unsigned port);
```

inw outw 函数用于读或写 16-bits 带宽 端口
```c
unsigned inw(unsigned port);
void outw(unsigned short word, unsigned port);
```

inl outl 函数用于读或写 32-bits 带宽 端口
```c
unsigned inl(unsigned port);
void outl(unsigned longword, unsigned port);
```

用户空间也提供了上述函数用于访问 I/O 端口, GNU lib c 将这些函数定义于 sys/io.h, 使用时需要遵循下列条件:
- 必须以 -O 编译程序, 以强制展开内联函数
- 必须先使用 ioperm 或 iopl 系统调用来取得 I/O 端口的访问权(ioperm 用户获得指定端口的权限, iopl 用于获取整个I/O空间的权限)
- 进程具有root身份, 或父进程已经用root身份获取I/O端口的访问权限

### 字符串操作
insb 用于从 8-bits 端口 port 读取 count 个字符, 并存放在 addr 指定的内存上; outsb 将 addr 指定的内存上的内容写入 count 字节到 port ; 
```c
unsigned insb(unsigned port, void *addr, unsigned long count);
void outsb(unsigned port, void *addr, unsigned long count);
```

insw 用于从 16-bits 端口 port 读取 count 个字符, 并存放在 addr 指定的内存上; outsw 将 addr 指定的内存上的内容写入 count 字节到 port ; 
```c
unsigned insw(unsigned port, void *addr, unsigned long count);
void outsw(unsigned port, void *addr, unsigned long count);
```

insl 用于从 32-bits 端口 port 读取 count 个字符, 并存放在 addr 指定的内存上; outsl 将 addr 指定的内存上的内容写入 count 字节到 port ; 
```c
unsigned insl(unsigned port, void *addr, unsigned long count);
void outsl(unsigned port, void *addr, unsigned long count);
```

## 使用 I/O 内存

使用 I/O 内存前, 必须先调用 request_mem_region 函数(定义与 linux/ioport.h)要求内核分配内存
```c
struct resource *request_mem_region(unsigned long start, unsigned long len, char *name);
```

release_mem_region 函数用于释放内存
```c
void release_mem_region(unsigned long start, unsigned long len);
```

check_mem_region 函数用于检查 I/O 内存是否被占用
```c
int check_mem_region(unsigned long start, unsigned long len);
```

分配好的内存不可以直接使用, 需要使用 ioremap 函数(定义于 asm/io.h )将 I/O 内存映射到一段虚拟地址
```c
void *ioremap(unsigned long phys_addr, unsigned long size);
void *ioremap_nocache(unsigned long phys_addr, unsigned long size);
void iounmap(void *virt_addr);
```
- ioremap_nocache 函数在大多数平台上与 ioremem 函数作用相同, 因为大多数平台上的 I/O 内存地址都出现再不可被缓存的地址上, 因此没必要实现 nocache 版的 ioremap 函数

ioread 函数用于读取 I/O 内存
```c
unsigned int ioread8(void *addr);
unsigned int ioread16(void *addr);
unsigned int ioread32(void *addr);
```
- addr 参数是 ioremap 函数返回的地址, 或在该地址上加一段偏移量

iowrite 函数用于写 I/O 内存
```c
void iowrite8(u8 value, void *addr);
void iowrite16(u16 value, void *addr);
void iowrite32(u32 value, void *addr);
```

ioread_rep iowrite_rep 函数用于连续重复读/写同一个 I/O 内存地址
```c
void ioread8_rep(void *addr, void *buf, unsigned long count);
void ioread16_rep(void *addr, void *buf, unsigned long count);
void ioread32_rep(void *addr, void *buf, unsigned long count);
void iowrite8_rep(void *addr, const void *buf, unsigned long count);
void iowrite16_rep(void *addr, const void *buf, unsigned long count);
void iowrite32_rep(void *addr, const void *buf, unsigned long count);
```
- count 指定重复的次数
- buf 指定存放读取数据的数组

memset 和 memcpy 函数用于读写一块连续的 I/O 内存
```c
memset_io(address, value, count);
memcpy_fromio(dest, source, nbytes);
memcpy_toio(dest, source, nbytes);
```
- count 指定读取的大小

ioport_map 函数可以将 I/O 端口映射到 I/O 内存, 这样就可以通过 ioread/iowrite 函数操作 I/O 端口
```c
void *ioport_map(unsigned long port, unsigned int count);
```

ioport_unmap 函数用来释放被分配给I/O端口映射的 I/O 内存 
```c
void ioport_unmap(void *addr);
```

