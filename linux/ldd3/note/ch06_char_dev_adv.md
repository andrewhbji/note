# 第六章 字符设备进阶操作

## ioctl 接口

## 阻塞和非阻塞 I/O

### 阻塞 I/O

## 轮询

## 异步编程

### 异步通知

### 异步I/O

## 定位操作

## 设备文件的访问控制

## ioctl 接口
驱动程序通过使用 file_operations 的 ioctl 接口函数向外部提供控制硬件的能力

在用户空间, 一般使用 ioctl 函数向硬件发命令
```c
int ioctl(int fd, unsigned long cmd, ...)
```

通常在 ioctl 函数额外传入第三个参数 arg, 如下
```c
int main(int argc, char **argv)
{
	int file_handler = 0;
	int cmd = 0;
	int arg = 0;
	if(argc < 4)
	{
		printf("Usage: ioctl <dev_file> <cmd> <arg>\n");
		return 0;
	}
	cmd = atoi(argv[2]);
	arg = atoi(argv[3]);
	printf("dev:%s\n", argv[1]);
	printf("cmd:%d\n", cmd);
	printf("arg:%d\n", arg);
	file_handler = open(argv[1], 0);

	ioctl(file_handler, cmd, arg);
	close(file_handler);
	return 0;
}
```

在内核空间, ioctl 接口用来接收用户空间调用 ioctl 函数传递进来的 cmd 和 arg
```c
int (*ioctl) (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
```
- cmd 对应用户空间传递进来的 cmd 参数, 作为 ioctl 命令
- arg 对应用户空间传递的第三个参数, 作为命令参数
>备注：因为用户空间的ioctl函数未对第三个参数的类型作任何限制, 所以在内核空间 arg 不一定要当成 unsigned long 来操作, 必要的时候需要强制转换

### cmd 参数
系统中每个 ioctl 命令的编号都是独一无二的, 为避免命令被下错, 需要规划cmd

cmd 值分为四个二进制段, 从低位到高位分别是
- number 序号, 长度为_IOC_NRBITS(一个字节)
- type 魔数, 长度为_IOC_TYPEBITS(一个字节)
- size 传输数据量, 长度为_IOC_SIZEBITS(14 bit)
- direction 传输方向, 长度为_IOC_DIRBITS(2 bit), 可能的值为_IOC_NONE(没有数据传输), _IOC_READ(只读), _IOC_WRITE(只写) 和 _IOC_READ|_IOC_WRITE (读写数据)

linux/ioctl.h 中定义一组宏用于编码和解析命令
```c
_IO(type,nr) /* 编码普通命令 */
_IOR(type, nre, datatype) /* 编码读数据的命令 */
_IOW(type,nr,datatype) /* 编码写数据的命令 */
_IOWR(type,nr,datatype) /* 编码双向传送的命令 */
_IOC_DIR(nr) /* 解析命令的传输方向 */
_IOC_TYPE(nr) /* 解析魔数 */
_IOC_NR(nr) /* 解析命令序号 */
_IOC_SIZE(nr) /* 解析传输数据量 */
```

### cmd 指令的命名规则
- S 代表 Set(设定), 需要传入一个指针
- T 代表 Tell(通知),  可直接使用参数值
- G 代表 "Get"(获取): 返回一个指向查询结果的指针
- Q 代表 "Query"(查询): 返回查询结果
- X 代表 "eXchange"(交换): 连续执行 G 和 S
- H 代表 "sHift"(移位): 连续执行 T 和 Q
```c
/* 这里使用 k 作为魔数, 不同的驱动程序应该使用不同的字符 */
#define SCULL_IOC_MAGIC 'k'

#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)

#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIC, 1, int)
#define SCULL_IOCSQSET _IOW(SCULL_IOC_MAGIC, 2, int)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET _IO(SCULL_IOC_MAGIC, 4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIC, 5, int)
#define SCULL_IOCGQSET _IOR(SCULL_IOC_MAGIC, 6, int)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET _IO(SCULL_IOC_MAGIC, 8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET _IOWR(SCULL_IOC_MAGIC,10, int)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET _IO(SCULL_IOC_MAGIC, 12)

#define SCULL_IOC_MAXNR 14
```

### 返回值
如果传入的cmd不能被识别, 一般返回 -ENOTTY, 或 -EINVAL

### 预定义的 ioctl 命令
少数指令(在 asm/ioctls.h 中预定义) 在被 ioctl 接口接收之前就被内核解码, 无法传入 ioctl 接口

预定义指令分为三大类
- 可用于任何文件(正常文件, 设备文件, FIFO, socket)
    - FIOCLEX 设置 close-on-exec 标志, 当进程以 exec() 系统调用运行另一个可执行文件时, 设置这个标志的可执行文件会被自动关闭
    - FIONCLEX 撤销 close-on-exec 标志
    - FIOQSIZE 用于返回一个文件或目录的大小, 不过如果用于设备文件, 会返回 ENOTTY
    - FIONBIO 用于修改 flip->f_flags 里的 O_NONBLOCK 标志, 在下这个指令时, ioctl() 函数的第三个参数必须注明是要设置还是要撤销这个标志
    
- 只能用于正常文件
- 只能用于特定类型的文件系统

### arg 参数
如果传入的arg参数是整型变量, 则可以直接使用, 如果传入的是指向用户空间内存的指针, 则需要将其强制转换为指针变量

除了 copy_from_user() 与 copy_to_user() 函数,  ioctl 接口也使用下面的函数和宏(均在asm/uaccess.h中定义)处理指向用户空间内存的指针
```c
int access_ok(int type, const void *addr, unsigned long size); 
put_user(datum, ptr)
__put_user(datum, ptr)
get_user(local, ptr)
__get_user(local, ptr)
```

#### access_ok 函数
access_ok 函数用于测试内存是否可访问, 如果可访问则返回1, 不能访问则返回0, 对于后者, ioctl 一般返回 -EFAULT给调用者
- type 参数取值为 VERIFY_READ 或 VERIFY_WRITE, 用于指定 access_ok 函数是被用来测试的内存可读还是可写
- addr 用于指定被测试内存的首地址
- size 用于指定被测试内存的大小, 以 byte 为计算单位
>备注: 
> 1. access_ok 函数只会检测 addr 指定的地址是否在进程的合理访问范围之内, 更确切的说, 就是确认 addr 没有侵犯到内核空间
> 2. 大部分驱动程序不需要刻意调用 access_ok 函数

#### put_user 宏
put_user 和 __put_user 宏用于将数据写入用户空间
- ptr 是用户空间内存的指针
- datum 指定要写入的数据, 其大小由 ptr 指针的类型决定
- put_user 宏会确认是否有资格写入指定的内存地址(使用 access_ok), 写入成功返回0, 失败返回-EFAULT
- __put_user 宏不会使用 access_ok 函数确认是否可以写入指定的内存地址, 需要主动使用 access_ok 函数检查目标内存

#### get_user 宏
get_user 和 get_user 宏用于将数据写入内核空间
- ptr 指向用户空间的地址
- local 是用于存放数据的变量

### 访问控制
Linux 内核需要对不同权限的执行单元进行访问控制

这些权限定义在 linux/capability.h, 不能定义新的权限, 除非修改内核源码

capable 函数用于检查执行单元的权限
```c
static inline bool capable(int cap)
```
- cap 参数取值如下
    - CAP_DAC_OVERRIDE 改变文件或目录的访问权限的能力
    - CAP_NET_ADMIN 执行网络管理的能力
    - CAP_SYS_MODULE 将模块载入/移出内核的能力
    - CAP_SYS_RAWIO 执行 raw I/O 操作能力, 例如访问设备的 I/O 端口, 直接和 USB 设备通信
    - CAP_SYS_ADMIN admin 权限
    - CAP_SYS_TTY_CONFIG 设置 tty 配置任务的能力
- 如果执行单元不具备权限, 返回 -EPREM

## 阻塞和非阻塞 I/O
在 Linux 中, 当进程处于休眠状态时, 会被移除出调度器的运行队列

对于休眠的进程, 需要注意:
- 休眠的时机: 原子操作时不可能休眠; 获取锁的时候不能休眠; 暂停了CPU中断时不能休眠； 获得互斥体时可以休眠, 但是休眠的时间应越短越好
- 从休眠状态恢复之后, 需要确认当初触发休眠的条件是否还存在
- 将进程从休眠状态唤醒的事件确保一定会发生

### 等待队列
Linux 中等待队列是使用"等待队列头节点" wait_queue_head_t 结构体(定义于linux/wait.h)来管理

DECLEAR_WAIT_QUEUE_HEAD init_waitqueue_head 宏用于初始化等待队列头节点
```c
DECLARE_WAIT_QUEUE_HEAD(name)
init_waitqueue_head(name)
```
- DECLARE_WAIT_QUEUE_HEAD 用于定义并初始化以 name 命名的 wait_queue_head_t 结构体
- init_waitqueue_head 用户初始化现有的 wait_queue_head_t 结构体变量

### 简易休眠
wait_event 宏用于让等待队列 queue 上的执行单元(进程)休眠
```c
wait_event(queue, condition)
wait_event_interruptible(queue, condition)
wait_event_timeout(queue, condition, timeout)
wait_event_interruptible_timeout(queue, condition, timeout)
```
- queue 是 wait_queue_head_t 变量
- condition 是个条件表达式, 当表达式为真时结束休眠
- timeout 用于指定超时时间(使用 jiffies 计数器计时), 超时后结束休眠

wake_up 宏用于唤醒 queue 上等待的所有执行单元(进程)
```c
wake_up(queue)
wake_up_interruptible(queue)
```
- queue 是wait_queue_head_t结构体的地址

wake_up 调用后, 休眠在 wait_event 上的执行单元会重新检查 condition, 当 condition 为真时, 则唤醒执行单元

### 阻塞操作和非阻塞操作
CPU 的运算速度快于外部设备的I/O速度, 所以驱动程序需要等待(休眠)的时候很多

允许休眠的操作被称为阻塞操作, 不允许休眠则被称为非阻塞操作

驱动程序通过 flip->f_flags 的 O_NONBLOCK 标志(定义在linux/fcntl.h, 由 linux/fs 引入), 如果 flip->f_flags 没有设置 O_NONBLOCK 位, 表示执行单元(进程)一般以执行完工作为最好准则, 无所谓是否休眠; 如果设置了这个标志位,  执行单元则以不休眠为最高准则

### 阻塞操作模式
驱动程序自带I/O缓冲区(环形队列)

输入缓冲区设置的目的是让执行单元不必等待就可以读写资料, 输出缓冲区用于接收进程写入设备的数据

输出缓冲区的设置可以减少进程间切换和用户/内核空间的切换的次数

- read: 当输入缓冲区为空, 没有数据可以提供给用户空间时, 需要让执行单元休眠, 直到输入缓冲区非空; 读取的数据量可以少于执行单元要求的量
- write: 当输出缓冲区已满, 则需要让执行单元休眠, 但是不能和 read 操作共用一个等待队列, 直到输出缓冲区有可写入空间; 写入的数据量可以少于执行单元要求的量

### 非阻塞操作模式
非阻塞操作模式主要是让进程可以根据设备的读写状态决定是否继续执行, 这种模式下, 如果输入缓冲区为空, 输出缓冲区已满, 执行单元则立即返回 -EAGAIN, 切换到用户空间让进程下次再试, 而不是让进程休眠

 O_NONBLOCK 标志位可能影响到 open 操作, 比如开启一个 FIFO 文件, 如果没有 writer 操作, 则 open 操作会被阻塞, 或者一个文件只能允许被一个进程 open, 其他进程再 open 救回被阻塞, 所以如果需要 open 操作支持 O_NONBLOCK,  则需要在可能造成阻塞的操作前, 返回 -EAGAGIN

### 进程休眠机制
wait_queue_head_t 结构体中使用一个 wait_queue_t 链表维护所有休眠中的执行单元

进程休眠的步骤
1. 初始化并配置 wait_queue_t 结构体, 并将其加入等待队列
2. 将执行单元的状态设为"即将休眠" 
3. 在让执行单元休眠之前, 需要检查休眠条件是否成立, 成立后再执行下面的步骤 
4. 调用 schedulue() 函数让调度器重新分配CPU, 这时, 处于等待队列中的执行单元就因无法分配到cpu而进入休眠状态

DEFINE_WAIT 和 init_wait 宏用于定义和初始化 wait_queue_t
```c
DEFINE_WAIT(name)
init_wait(wait)
```
- DEFINE_WAIT 用于定义并初始化以 name 命名的 wait_queue_t 结构体
- init_wait 用户初始化现有的 wait_queue_t 结构体变量

prepare_to_wait 函数用于将 wait_queue_t 加入等待队列, 并设置执行单元的状态
```c
void prepare_to_wait(wait_queue_head_t *queue, wait_queue_t *wait, int state);
```
- queue 和 wait 分别是 wait_queue_t 和 wait_queue_head_t 的指针
- state 用于指示执行单元的状态, 可取值为 TASK_RUNNING(可运行状态) TASK_INTERRUPTIBLE(可中断的休眠状态) TASK_UNINTERRUPTIBLE(不可中断的休眠状态)

schedule 函数用于通知调度器重新分配CPU
```c
void schedule(void);
```

finish_wait 函数用于在执行单元从休眠状态恢复后将 wait_queue_t 从等待队列移除
```c
void finish_wait(wait_queue_head_t *q, wait_queue_t *wait);
```
>备注: wake_up 函数会将等待队列理的所有执行单元的状态都改成 TASK_RUNNING,  调度器下一次分配CPU时, 这些执行单元都会被唤醒

### 独占等待
独占等待是指等待队列的执行单元中, 只有一个执行单元需要从休眠状态中恢复

Linux 中, 将 wait_queue_t.flags 设置为 WQ_FLAG_EXCLUSIVE 表示执行单元是独占等待的, 它会被加到待命队列的最后, 而普通的 wait_queue_t 会被放到队首; 当 wake_up 函数处理等待队列时, 如果有设置为WQ_FLAG_EXCLUSIVE的 wait_queue_t, 就不会在处理其他的 wait_queue_t

适用独占等待的情况
- 资源有限但竞争严重
- 一个进程就足以消耗全部资源, 如 apache server

prepare_to_wait_exclusive 用于将wait_queue_t设置为独占等待, 它会自动将wait_queue_t.flags设置为WQ_FLAG_EXCLUSIVE
```c
void prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_t *wait, int state);
```

### 其他wake_up函数的变体
```c
wake_up_interruptible(wait_queue_head_t *queue);
wake_up_nr(wait_queue_head_t *queue, int nr);
wake_up_interruptible_nr(wait_queue_head_t *queue, int nr);
wake_up_all(wait_queue_head_t *queue);
wake_up_interruptible_all(wait_queue_head_t *queue);
wake_up_interruptible_sync(wait_queue_head_t *queue);
```
- *_interruptible 表示不会唤醒设置为 TASK_UNINTERRUPTIBLE 的执行单元
- *_nr 表示唤醒 nr 指定个数独占等待的执行单元, 如果 nr 为 0, 则唤醒所有独占等待的执行单元
- *_all 表示唤醒等待队列中的所有执行单元, *_interruptible_all 表示唤醒所有不是设置为 TASK_UNINTERRUPTIBLE 的执行单元
- *_sync 是其他 wake_up 函数的原子操作版本, 等价与在原子操作环境中调用 wake_up 函数

## 轮询
用户空间的 select poll epoll 系统调用本质上都使用 file_operations 的 poll 接口函数
```c
unsigned int (*poll) (struct file *filp, poll_table *pt); 
```

当进程调用 select poll epoll, 内核便会执行设备相关驱动程序的 poll 接口操作

实现 poll 接口必须完成下面两个任务:
1. 调用一次或多次 poll_wait 函数, 将当前执行单元挂在某个等待队列上
2. 返回当前设备状态掩码

poll 系统调用对应的内核函数 sys_poll(定义于 fs/select.c)会执行一个循环, 这个循环执行如下：
1. 回调 poll 接口函数, 将 poll 接口函数返回的状态掩码和 pollfd->events 值进行逻辑与运算, 将结果记录到 polfd->revents 中, 并返回结果
2. 如果设备状态就绪(第一步返回值为非0值), 如果就绪则跳出循环并返回就绪设备数量
3. 如果设备状态没有就绪, 让执行单元在 poll_wait 注册的等待队列上休眠一段时间
4. 执行单元被唤醒的条件:
    - 休眠超时
    - 有操作将上一步休眠的执行单元唤醒
5. 继续执行 1 , 然后无论设备是否就绪都返回就绪设备数量

>备注: Linux poll 机制分析参考 http://www.embeddedlinux.org.cn/html/yingjianqudong/201303/11-2476.html

### poll_wait 函数
poll_wait 函数在 linux/poll.h 中定义, 将一个或多个能唤醒执行单元, 改变轮询状态的等待队列放入轮询表 poll_table
```c
void poll_wait(struct file *flip, wait_queue_head_t *queue, poll_table *pt);
```
- queue 指定等待队列, 由驱动程序提供
- pt 指定 poll_table, 由 poll 接口函数传入

### 设备状态掩码
linux/poll.h 中预定义了一些用来描述设备状态的标志位
- POLLIN 表示设备可读
- POLLRDNORM 表示普通设备已经准备好被读取, 一般和 POLLIN 结合使用(POLLIN | POLLRDNORM)
- POLLRDBAND 表示设备可读出紧急数据
- POLLPRI 表示设备可读出高优先级数据, select 系统调用将高优先级数据解释为异常情况, 所以POLLPRI一般让调用者认为文件异常
- POLLHUP 表示设备已经读到末尾
- POLLERR 表示设备出错
- POLLOUT 表示设备可以被写入
- POLLWRNORM 表示普通设备准备好被写入, 一般和 POLLOUT 结合使用(POLLOUT | POLLWRNORM)
- POLLWRBAND 表示优先级不为0的数据可以被写入

## 读 写 轮询的操作准则
从设备读取数据
- 输入缓冲区中有数据, 则 read 操作不应延迟, poll 操作返回 POLLIN|POLLRDNORM
- 输入缓冲区空, 如果进程没有设置设备文件的O_NONBLOCK标志, read 操作应阻塞, 直到输入缓冲区可读; 如果设置O_NONBLOCK标志, read 操作应立即返回 -EAGAIN, 此时 poll 操作返回的mask应将 POLLIN 位和 POLLRDNORM 归零
- 如果读到文件尾, read 操作应返回 0 (无论是否设置设备文件的O_NONBLOCK标志), poll 操作应返回 POLLHUP

将数据写入设备
- 输出缓冲区有空间, write 操作不应延迟, poll 操作返回 POLLOUT|POLLWRNORM
- 输出缓冲区满, 如果进程没有设置设备文件的O_NONBLOCK标志, write 操作应阻塞, 直到输出缓冲区有空间; 如果设置O_NONBLOCK标志, write 操作应立即返回 -EAGAIN, 此时 poll 操作返回的mask应将 POLLOUT 位和 POLLWRNORM 归零; 如果设备不能再继续接收更多数据, write 操作应返回 -ENOSPC(设备空间已满)
- 在使用 select 或 poll 预先判断下一次是否可读或可写后, read 和 write 操作必须要配合相应的非阻塞的效果

刷新挂起的数据
fsync 操作用来将写入输出缓冲区的数据写入到硬件设备上
```c
int (*fsync) (struct file *file, struct dentry *dentry, int datasync); 
```
- datasync 用来区别 fsync 和 fdatasync 系统调用, 只有文件系统的内部需要关心这两种系统调用的差异
- 字符驱动不需要实现, 块设备驱动一般使用 block_fsync() 函数

## 异步编程

### 异步通知
异步通知是通过由内核向用户空间应用程序发送 SIGIO 信号来实现的

应用程序必须作一下工作:
1. 设置 SIGIO 信号处理函数
2. 告诉内核需要接收通知的进程ID
```c
fcntl(STDIN_FILENO, F_SETOWN, getpid());
```
3. 设置 FASYNC 标志, 内核会通过驱动调用 fasync 函数为以后的信号通知做准备
```c
fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | FASYNC);  
```

在应用程序做完上面设置后, 就可以做别的其他事, 然后内核会通过发出 SIGIO 信号来通知应用, 然后应用对 SIGIO 进行信号处理

在内核空间, 相关的驱动程序需要实现:
1. 定义一个全局的 struct fasync_struct 指针
2. 实现 file_operations 的 fasync 接口函数, 在其中调用内核的 fasync_helper 函数
3. 在驱动的某个可以获知数据可访问的函数中(如 read 函数)调用 kill_fasync 函数向用户空间发 SIGIO 信号

#### fasync 函数
```c
- int (*fasync) (int fd, struct file *filp, int mode);
```

用户空间执行 fcntl(STDIN_FILENO, F_SETOWN, pid) 时, 内核会将 filp->f_owner 设置为 pid

用户空间执行 fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | FASYNC)时, 内核会执行 fasync 函数

#### 通知机制的实现
Linux 内核实现通知机制主要由 fasync_struct 结构体以及 fasync_helper 和 kill_fasync 函数来实现

在 fcntl 函数被执行的时候, 调用 fasync_helper 函数完成异步通知的准备工作

fasync_helper 函数会将执行单元(进程)的ID移入或移除内核的通知名单
```c
int fasync_helper(int fd, struct file *filp, int mode, struct fasync_struct **fa);
```
- fd filp mode 参数对应 fasync 函数的三个参数

kill_fasync 函数用于向通知名单中的进程发信号
```c
void kill_fasync(struct fasync_struct **fa, int sig, int band);
```
- fa 指定 fasync_struct 结构体的指针
- sig 指定信号, 用于异步通知的信号为SIGIO
- band 指定 IO 的方向, 读取数据时使用 POLL_IN, 写入数据时使用 POLL_OUT

当文件关闭时, 也需要显示调用 fasync 函数将执行单元(进程)的ID移除内核的通知名单, fd 参数传入 -1, mode 参数传入 0

### 异步I/O

## 定位操作
进程发生 lseek() 或 llseek() 系统调用时, 会触发驱动程序的 file_operations 的 llseek 接口函数
```c
loff_t llseek(struct file *filp, loff_t off, int whence);
```
- whence 指定移动文件指针的参考点, 0 即 SEEK_SET, 参考点位是文件起始位置; 1 即 SET_CUR, 参考点位是文件指针当前的位置; 2 即 SET_END, 参考点位于文件结束位置

如果设备不需要定位操作, 需要在 open 接口函数中调用 nonseekable_open 函数(在 linux/fs.h中定义), 然后将 file_operations 的 llseek 函数指针指向 no_lseek 函数(在 linux/fs.h中定义)
```c
loff_t no_llseek(struct file *file, loff_t offset, int whence);
int nonseekable_open(struct inode * inode, struct file * filp);
```

## 设备文件的访问控制
对设备提供访问控制能力, 是保证设备节点可靠性的关键, 访问控制方式依据限制身份, 限制进程访问数量, 限制手段等, 划分为:
- 单一进程独占
- 单用户独占
- 阻塞 open 操作
- 在 open 时复制设备

### 单一进程独占
单一进程独占一般使用 atomic_t 类型的变量, 其值初始化为 1, 第一个进程 open 成功后, 将此值减为 0; 这样其他进程 open 时将无法通过 atomic_t 变量是否为 0 的测试, 这时返回 -EBUSY

另外, 在 release 时, 需要将 atomic_t 变量复位为 1, 以便下一个进程可以开启
```c
static atomic_t available = ATOMIC_INIT(1);
static int open(struct inode *inode, struct file *filp)
{
        if (! atomic_dec_and_test (&available))
        {
                atomic_inc(&available);
                return -EBUSY; /* already open */
        }
        ...
}

static int scull_s_release(struct inode *inode, struct file *filp)
{
        atomic_inc(&available); /* release the device */
        return 0;
}
```

### 单用户独占
这种访问策略允许一个用户的多个进程同时访问设备, 但是不允许多个用户同时访问设备. 与独享设备的策略相比, 这种方法更加灵活. 此时需要增加两个数据项, 一个打开计数器和一个设备属主UID
- 第一次 open 设备时, 设备无条件打开, 保存访问设备的进程的ID, 然后将计数器加一
- 此后每次 open, 只要计数器没有降为0, 并且执行单元不具备 root 权限, 就检查进程的UID和EUID是否和第一次打开进程一致, 不一致则返回 -EBUSY
- 如果执行单元具备 root 权限, 则无条件开放设备, 并且将计数器加一
- release 设备时, 将计数器减一
```c
static int open(struct inode *inode, struct file *filp)
{
	spin_lock(&lock);

	if (count && 
			(!uid_eq(owner , current_uid()) ) &&  /* allow user */
			(!uid_eq(owner , current_euid()) ) && /* allow whoever did su */
			!capable(CAP_DAC_OVERRIDE)) { /* still allow root */
		spin_unlock(&lock);
		return -EBUSY;   /* -EPERM would confuse the user */
	}

	if (count == 0)
		owner = current_uid(); /* grab it */

	count++;
	spin_unlock(&lock);
    
    ...
}

static int release(struct inode *inode, struct file *filp)
{
	spin_lock(&lock);
	count--; /* nothing else */
	spin_unlock(&lock);
	return 0;
}
```
### 阻塞 open 操作
上面两种访问控制方法当设备不能访问时, 都是返回-EBUSY退出, 但是有些情况下, 可能需要让进程阻塞等待, 这时就需要实现阻塞型open.此时需要维护一个等待队列、计数器、设备属主UID
- 首先判断 open 文件时是否设置 O_NONBLOCK, 如果设置则返回 -EAGAIN
- 然后让执行单元进入休眠状态, 并设置唤醒条件
- release 文件时, 需要判断计数器是否归零, 归零后唤醒等待队列上休眠的执行单元
```c
static int scull_w_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = &scull_w_device; /* device information */

	spin_lock(&scull_w_lock);
	while (! scull_w_available()) {
		spin_unlock(&scull_w_lock);
		if (filp->f_flags & O_NONBLOCK) return -EAGAIN;
		if (wait_event_interruptible (scull_w_wait, scull_w_available()))
			return -ERESTARTSYS; /* tell the fs layer to handle it */
		spin_lock(&scull_w_lock);
	}

	if (scull_w_count == 0)
		scull_w_owner = current_uid(); /* grab it */

	scull_w_count++;
	spin_unlock(&scull_w_lock);
    
    ...
}

static int scull_w_release(struct inode *inode, struct file *filp)
{
	int temp;

	spin_lock(&scull_w_lock);
	scull_w_count--;
	temp = scull_w_count;
	spin_unlock(&scull_w_lock);

	if (temp == 0)
		wake_up_interruptible_sync(&scull_w_wait); /* awake other uid's */
	return 0;
}
```

### 在 open 时复制设备
最后一种策略时在不同的进程开启设备时, 产生专属这个进程的设备副本. 正常的硬件设备不适用这种策略, 只适用于 tty 等软件模拟设备
- 首先在 open 时找出正确的虚拟设备, 如果没有, 就产生一个
- release 时, 如果计数器没有归零, 则不做任何处理, 所以下次 open 时扔可以找到这个设备
- 只在计数器归零后, 或在 cleanup 时统一释放虚拟设备
