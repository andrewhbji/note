# 第四章 调试技术

[TOC]
## 内核对调试的支持
## 打印调试
## 查询调试
## 观察调试
## 调试系统故障
## debuger 和 相关工具

## 内核对调试的支持
make menuconfig 界面中的 kernel hacking 选项中包含对内核调试的支持
- CONFIG_DEBUG_KERNEL -> kernel hacking->Kernel debugging

    打开/关闭调试功能
    
- CONFIG_DEBUG_SLAB -> kernelhacking->Debug slab memory allocations

    打开/关闭对内核内存分配函数的检查, 用于找出内存越界或没有初始化之类的错误. 打开这个选项后, 被分配的内存的每一个字节都会被设置为0xa5, 然后才交给调用者; 当内存被释放, 则设置为0x6b. 通过检查驱动程序的输出结果或 oops 信息中是否重复出现这些内存值, 就可以猜测出错误是什么; 对于每一块被分配的内存, 内核也会将其前后边界设置为特殊的保护值, 如果这些保护值被改变了, 内核就会知道有内存越界, 并且输出警告; 此外还有各种细致的检查也会生效
    
- CONFIG_DEBUG_PAGEALLOC -> kernelhacking->Debug page memory allocations

    当释放内存分页时, 将完整的内存分页从内核地址空间中移除. 这会严重影响性能, 但会帮助我们快速找出某些类型的内存错乱问题
    
- CONFIG_DEBUG_SPINLOCK -> kernelhacking->Spinglock and rw-lock debugging:basic checks

    开启后, 内核会捕捉对为初始化的自旋锁的操作, 以及其他各种错误, 比如重复解锁两次等

- CONFIG_DEBUG_SPINLOCK_SLEEP -> kernel hacking->Spinglock debugging:sleep-inside-spinlockchecking
    
    开启后, 若持有自旋锁的函数试图休眠, 或调用了可能会造成休眠的另一个函数(即使没有休眠), 都会导致内核发出警告

- CONFIG_INIT_DEBUG

    开启后, 在系统初始化或模块装在完毕后不会真正丢弃以 __init 或 __initdata 标示的数据或函数, 但是会检查任何在初始化完成之后试图读写这些数据或调用这些函数的行为
    
- CONFIG_DEBUG_INFO -> kernelhacking->Compile the kernel with debug info

    开启后, 构建出来的内核会包含有完整的调试信息, 用于使用 gdb 调试内核; 如果需要使用 gdb 调试内核, 还需要开启 CONFIG_FRAME_POINTER 选项
    
- CONFIG_MAGIC_SYSRQ -> Kernel-->hacking Magic SysRq key

    开启/关闭 magic SysRq 按钮
    
- CONFIG_DEBUG_STACKOVERFLOW -> General setup -->Check forstack overflows ; CONFIG_DEBUG_STACK_USAGE -> kernel hacking->Stackutilization instrumentation

    这两个选项用于协助追踪核心堆栈溢出. CONFIG_DEBUG_STACKOVERFLOW 开启后, 内核会在每次堆栈操作后都会检查是否发生溢出; CONFIG_DEBUG_STACK_USAGE 开启后, 内核会监控堆栈使用量, 并通过 magic SysRq 按钮提供这些信息

- CONFIG_KALLSYMS -> General setup -->Configure standard kernel features(for smallsystems)--->Load all symbols for debugging/ksymoops

    开启后, 构建出来的内核含有内核符号信息, 供调试环境使用; 如果没有这些信息, oops 信息理的追溯路径是以十六进制数表示, 这对 debuger 来说没有任何意义

- CONFIG_IKCONFIG -> General setup -->Kernel .config support CONFIG_IKCONFIG_PROC ; CONFIG_ACPI_DEBUG -> Powermanagement and ACPI options -->ACPI (Advanced Configuration and PowerInterface) Support-->Debug Statements
    
    这两个选项开启后, 内核会包含完整的配置信息, 通过 /proc 可以使用这些信息

- CONFIG_ACPI_DEBUG -> Powerman agement and ACPI options -->ACPI (Advanced Configuration and PowerInterface) Support-->Debug Statements

    这个选项可以使 APCI 子系统输出完整的调试信息, 用于调试 ACPI 子系统以及相关功能

- CONFIG_DEBUG_DRIVER -> Device Drivers-->Generic Driver Options-->Driver Core verbose debug message

    用于打开 driver core 的调试信息, 一边追踪发生在底层代码中的问题

- CONFIG_SCSI_CONSTANTS -> Device Drivers-->SCSI device support-->Verbose SCSI error reporting (kernel size +=12K)

    用于使 SCSI 子系统输出详细的调试信息, 用于调试 SCSI 子系统以及相关功能
    
- CONFIG_INPUT_EVBUG -> Device Drivers-->Input device support--->Event debugging

    开启后, 让用户空间的 loger 可收到input事件的信息, 用于调试input设备相关的驱动程序

- CONFIG_PROFILING -> General steup---> Profile system profiling(实验性的)
    
    用于分析系统性能, 也可以追踪一些内核挂起和相关的问题
    
## 打印调试
### printk 函数
```c
#include <linux/printk.h>
int printf(const char *restrict level ,const char *restrict format, ...);
```
- level 用来指示信息的级别, 取值(定义在linux/kern_levels.h)包括:
    - KERN_EMERG 用于紧急消息, 常常是那些崩溃前的消息.
    - KERN_ALERT 需要立刻动作的情形.
    - KERN_CRIT 严重情况, 常常与严重的硬件或者软件失效有关.
    - KERN_ERR 用来报告错误情况; 设备驱动常常使用 KERN_ERR 来报告硬件故障.
    - KERN_WARNING 有问题的情况的警告, 这些情况自己不会引起系统的严重问题.
    - KERN_NOTICE 正常情况, 但是仍然值得注意. 在这个级别一些安全相关的情况会报告.
    - KERN_INFO 信息型消息. 在这个级别, 很多驱动在启动时打印它们发现的硬件的信息.
    - KERN_DEBUG 用作调试消息.
- format 和 参数列表与 printf 一致
- 没有指定 level 时， printk语句采用默认级别 DEFAULT_MESSAGE_LOGLEVEL , 通常是 KERN_WARNING

根据日志级别, 内核可能会把消息打印到当前控制台上. 控制台也有级别，用整数变量console_loglevel来表示. 当日志级别的值小于 console_loglevel, 且 klogd 运行时, 消息才能显示出来. 同样, console_loglevel 也有一个默认级别 DEFAULT_CONSOLE_LOGLEVEL. 

修改 console_loglevel 的方法有以下几种:
- 通过 sys_syslog 系统调用
    a. 先杀掉klogd进程, 然后再用-c选项重启它的同时修改console_loglevel的值
    b. 使用 klogctl 函数
    ```c
    #include <sys/klog.h>
    extern int klogctl (int __type, char *__bufp, int __len) __THROW;
    ```
- 在命令行中修改 /proc/sys/kernel/printk 文件
```sh
#  echo 1 > /proc/sys/kernel/printk
#  insmod hello.ko       ;看不到打印消息
#  rmmod hello
#  echo 8 > /proc/sys/kernel/printk
#  insmod hello.ko       ;消息显示出来了
Hello,world
#  rmmod hello
Goodbye,world 
```

## 查询调试
大量使用 printk 会拖慢系统, 因为 syslogd 或 klogd 会不停地将打印信息同步到文件, 因此 Linux 可以通过读取如 /proc 等虚拟文件系统的方式查询日志

### /proc 文件系统
/proc 下的文件可以用于向用户空间提供内核模块的信息

### proc_create 函数
proc_create 函数用于创建 /proc 文件
```c
#incluce <linux/proc_fs.h>
struct proc_dir_entry *proc_create(const char *name, umode_t mode, struct proc_dir_entry *parent, const struct file_operations *proc_fops);
struct proc_dir_entry *proc_create_data(const char *name, umode_t mode, struct proc_dir_entry *parent, const struct file_operations *proc_fops, void *data);
返回值: 成功返回的 proc_dir_entry 结构体代表当前 proc 文件路径; 失败返回 NULL
```
- *name 指定文件名, 也可以指定为相对路径, 表示在现有的路径中创建 /proc 文件, 如 “/driver/scullmem” 表示在 /proc/driver 下创建 scullmem 文件
- mode 指定文件权限掩码, 传入 0 代表使用系统默认值
- *parent 指定父路径, 设置为NULL代表在 /proc 下创建; proc_dir_entry 结构体一般使用 proc_mkdir 函数生成
- *proc_fops 指定 /proc 文件的 file_operations， 一般在 open 函数中使用 seq_open 函数注册 seq_operations 结构体
```c
static struct seq_operations scull_seq_ops = {
	.start = scull_seq_start,
	.next  = scull_seq_next,
	.stop  = scull_seq_stop,
	.show  = scull_seq_show
};

static int scull_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &scull_seq_ops);
}

static struct file_operations scull_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = scull_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
```
- data 是传入 seq_file

### seq_operations 和 seq_file 结构体
读取 /proc 文件时, seq_read 函数通过使用 seq_operations 和 seq_file (均在 linux/seq_file.h中定义) 完成对 /proc 文件的读取过程, seq_file 代表 /proc 文件, seq_operations 用于指定对 /proc 文件的操作
```c
struct seq_file {
	char *buf;          //在seq_open中分配，大小为4KB
	size_t size;        //4096
	size_t from;        //struct file 从 seq_file 中向用户空间缓冲区拷贝时相对于buf的偏移地址
	size_t count;       //可以拷贝到用户空间的字符数目
	size_t pad_until;
	loff_t index;       //从内核空间向seq_file的内核空间缓冲区buf中拷贝时start、next的处理的下标pos数值，即用户自定义遍历iter
	loff_t read_pos;    //当前已拷贝到用户空间的数据量大小，即struct file中拷贝到用户空间的数据量
	u64 version;
	struct mutex lock;  //保护该 seq_file 的互斥锁结构
	const struct seq_operations *op;    //seq_start,seq_next,set_show,seq_stop 函数结构体
	void *private;
};

struct seq_operations {
	void * (*start) (struct seq_file *m, loff_t *pos);
	void (*stop) (struct seq_file *m, void *v);
	void * (*next) (struct seq_file *m, void *v, loff_t *pos);
	int (*show) (struct seq_file *m, void *v);
};
```
- start 函数用于指定并返回需要读取内存的起始位置； 如果 *pos 指定的位置超过了内存实际位置, 或超出了 /proc 文件大小, 应当返回 NULL ; 也可以返回 SEQ_START_TOKEN, 用于让 show 函数输出文件头, 但这只能在 pos 为 0 时使用
- next 函数用于把 seq 文件的当前读位置移动到下一个读取位置, 然后返回下一次将要读取的内存位置； 如果内存已经读完, 或超出 /proc 文件大小, 返回 NULL ; 
- stop 函数用于在读取完 /proc 文件后调用, 类似于文件的 close 系统调用, 用于做一些必要的清理, 如释放内存
- show 用于将内核内存的内容输出到 /proc, 成功返回0, 失败返回错误码

seq_read 执行时, 通常以 start->show->next->show...->next->show->next->stop 的形式调用 seq_operations 的注册的函数 , 直到处理完全部内存信息;

下面函数用于将内容输出到 seq_file
```c
/*函数seq_putc用于把一个字符输出到seq_file文件*/
int seq_putc(struct seq_file *m, char c);
  
/*函数seq_puts则用于把一个字符串输出到seq_file文件*/
int seq_puts(struct seq_file *m, const char *s);

/*函数seq_escape类似于seq_puts，只是，它将把第一个字符串参数中出现的包含在第二个字符串参数中的字符按照八进制形式输出，也即对这些字符进行转义处理*/
int seq_escape(struct seq_file *, const char *, const char *);
  

/*函数seq_printf是最常用的输出函数，它用于把给定参数按照给定的格式输出到seq_file文件*/
int seq_printf(struct seq_file *, const char *, ...)__attribute__ ((format(printf,2,3)));

/*函数seq_path则用于输出文件名，字符串参数提供需要转义的文件名字符，它主要供文件系统使用*/
int seq_path(struct seq_file *, struct vfsmount *, struct dentry *, char *);
```

### single_open 函数
如果需要处理的内存一次就可以读取完毕, 则不需要定义 seq_operations 结构体, 只需要在 file_operations 的 open 函数中注册 show 函数即可
```c
int proc_show (struct seq_file *m, void *v);

static int scull_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_show, NULL);
}

static struct file_operations scull_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = scull_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};
```

使用 single_open 函数时, file_operations 的 release 函数必须使用 single_release

## 观察调试
strace命令是一个集诊断、调试、统计与一体的工具，我们可以使用strace对应用的系统调用和信号传递的跟踪结果来对应用进行分析，以达到解决问题或者是了解应用工作过程的目的。

strace的最简单的用法就是执行一个指定的命令，在指定的命令结束之后它也就退出了。在命令执行的过程中，strace会记录和解析命令进程的所有系统调用以及这个进程所接收到的所有的信号值。

### 语法
```
strace [ -dffhiqrtttTvxx ] [ -acolumn ] [ -eexpr ] ... [ -ofile ] [-ppid ] ... [ -sstrsize ] [ -uusername ] [ -Evar=val ] ... [ -Evar ]... [ command [ arg ... ] ] strace -c [ -eexpr ] ... [ -Ooverhead ] [ -Ssortby ] [ command [ arg... ] ]
```

### 选项
- -c 统计每一系统调用的所执行的时间,次数和出错的次数等. 
- -d 输出strace关于标准错误的调试信息.
- -f 跟踪由fork调用所产生的子进程.
- -ff 如果提供-o filename,则所有进程的跟踪结果输出到相应的filename.pid中,pid是各进程的进程号.
- -F 尝试跟踪vfork调用.在-f时,vfork不被跟踪.
- -h 输出简要的帮助信息.
- -i 输出系统调用的入口指针.
- -q 禁止输出有关附加、分离的消息.
- -r 打印每一个系统调用的相对时间
- -t 在输出中的每一行前加上时间信息.
- -tt 在输出中的每一行前加上时间信息,微秒级.
- -ttt 微秒级输出,以秒了表示时间.
- -T 显示每一个系统调用所耗的时间.
- -v 输出所有系统调用。默认情况下，一些频繁使用的系统调用不会输出
- -V 输出strace的版本信息.
- -x 以十六进制形式输出非标准字符串
- -xx 所有字符串以十六进制形式输出.
- -a column 设置返回值的输出位置.默认为40.
- -e expr 指定一个表达式,用来控制如何跟踪.格式：[qualifier=][!]value1[,value2]... qualifier只能是 trace,abbrev,verbose,raw,signal,read,write其中之一.value是用来限定的符号或数字.默认的 qualifier是 trace.感叹号是否定符号.例如:-eopen等价于 -e trace=open,表示只跟踪open调用.而 -e trace!=open 表示跟踪除了open以外的其他调用.有两个特殊的符号 all 和 none. 注意有些shell使用!来执行历史记录里的命令,所以要使用\\\\.
- -e trace=set 只跟踪指定的系统调用.例如:-e trace=open,close,rean,write表示只跟踪这四个系统调用.默认的为set=all.
- -e trace=file 只跟踪有关文件操作的系统调用.
- -e trace=process 只跟踪有关进程控制的系统调用.
- -e trace=network 跟踪与网络有关的所有系统调用.
- -e strace=signal 跟踪所有与系统信号有关的 系统调用
- -e trace=ipc 跟踪所有与进程通讯有关的系统调用
- -e abbrev=set 设定strace输出的系统调用的结果集.-v 等与 abbrev=none.默认为abbrev=all.
- -e raw=set 将指定的系统调用的参数以十六进制显示.
- -e signal=set 指定跟踪的系统信号.默认为all.如 signal=!SIGIO(或者signal=!io),表示不跟踪SIGIO信号.
- -e read=set 输出从指定文件中读出的数据.例如: -e read=3,5 
- -e write=set 输出写入到指定文件中的数据.
- -o filename 将strace的输出写入文件filename
- -p pid 跟踪指定的进程pid.
- -s strsize 指定输出的字符串的最大长度.默认为32.文件名一直全部输出.
- -u username 以username的UID和GID执行被跟踪的命令

### 实例
使用 gcc -o test test.c 编译下面程序
```c
#include <stdio.h>
int main() { 
    int a; 
    scanf("%d", &a); 
    printf("%09d\n", a); 
    return 0;
}
```

使用 strace 执行 test
```sh
$ strace ./test
execve("./test", ["./test"], [/* 41 vars */]) = 0
uname({sys="Linux", node="orainst.desktop.mycompany.com", ...}) = 0
brk(0) = 0x8078000 fstat64(3, {st_mode=S_IFREG|0644, st_size=65900, ...}) = 0 
old_mmap(NULL, 65900, PROT_READ, MAP_PRIVATE, 3, 0) = 0xbf5ef000 
close(3) = 0 
open("/lib/tls/libc.so.6", O_RDONLY) = 3 
read(3, "\177ELF\1\1\1\0\0\0\0\0\0\0\0\0\3\0\3\0\1\0\0\0\200X\1"..., 512) = 512 
fstat64(3, {st_mode=S_IFREG|0755, st_size=1571692, ...}) = 0 
old_mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0xbf5ee000 
old_mmap(NULL, 1275340, PROT_READ|PROT_EXEC, MAP_PRIVATE, 3, 0) = 0xa02000 
old_mmap(0xb34000, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, 3, 0x132000) = 0xb34000 
old_mmap(0xb37000, 9676, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0xb37000 
close(3) = 0 
set_thread_area({entry_number:-1 -> 6, base_addr:0xbf5ee740, limit:1048575, seg_32bit:1, contents:0, read_exec_only:0, limit_in_pages:1, seg_not_present:0, useable:1}) = 0 
munmap(0xbf5ef000, 65900) = 0 fstat64(0, {st_mode=S_IFCHR|0620, st_rdev=makedev(136, 0), ...}) = 0 
mmap2(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0xbf5ff000 
read(0, 99 "99\n", 1024) = 3 
fstat64(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(136, 0), ...}) = 0 
mmap2(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0xbf5fe000 
write(1, "000000099\n", 10000000099 ) = 10 munmap(0xbf5fe000, 4096) = 0 exit_group(0) = ?
```
从trace结果可以看到，系统首先调用execve开始一个新的进行，接着进行些环境的初始化操作，最后停顿在”read(0,”上面，这也就是执行到了我们的scanf函数，等待我们输入数字呢，在输入完99之后，在调用write函数将格式化后的数值”000000099″输出到屏幕，最后调用exit_group退出进行，完成整个程序的执行过程

### 追踪信号传递

先strace ./test，等到等待输入的画面的时候不要输入任何东西，然后打开另外一个窗口，输入如下的命令
```sh
killall test
```
程序退出后, trace 输出
```sh
read(0, 0xbf5ff000, 1024) = ? ERESTARTSYS (To be restarted)
--- SIGTERM (Terminated) @ 0 (0)
--- +++ killed by SIGTERM +++
```

## 调试系统故障
###  Oops 信息
因为不当使用指针引起的bug, 一般会输出 Oops 信息到内核日志中, 需使用 dmesg 查看

如下导致内存溢出的代码
```c
ssize_t faulty_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
    int ret;
    char stack_buf[4];

    /* stack_buf 只有4个字节, 这里填充20个字节的Oxff会导致内存移除 */
    memset(stack_buf, 0xff, 20);
    if (count > 4)

        count = 4; /* copy 4 bytes to the user */
    ret = copy_to_user(buf, stack_buf, count);
    if (!ret)

        return count;
    return ret;
}
```
其Oops信息如下：
```sh
EIP: 0010:[<00000000>]
Unable to handle kernel paging request at virtual address ffffffff

printing eip:  
ffffffff  
Oops: 0000 [#5]  
SMP  
CPU:  0  
EIP:  0060:[<ffffffff>]  Not tainted  
EFLAGS: 00010296  (2.6.6)  
EIP is at 0xffffffff  
eax: 0000000c  ebx: ffffffff  ecx: 00000000  edx: bfffda7c  
esi: cf434f00  edi: ffffffff  ebp: 00002000  esp: c27fff78  
ds: 007b  es: 007b  ss: 0068  

Process head (pid: 2331, threadinfo=c27fe000 task=c3226150) 
Stack: ffffffff bfffda70 00002000 cf434f20 00000001 00000286 cf434f00 fffffff7 bfffda70 c27fe000 c0150612 cf434f00 bfffda70 00002000 cf434f20 00000000 00000003 00002000 c0103f8f 00000003 bfffda70 00002000 00002000 bfffda70 
Call Trace: [<c0150612>] sys_read+0x42/0x70 [<c0103f8f>] syscall_call+0x7/0xb 
Code: Bad EIP value. 
```
- EIP 或 RIP 信息会指出出错的指令位置或函数调用. 如 EIP is at faulty_write+0x4/0x10 [faulty] 这段信息指向了  faulty_write 函数的第4个字节, 指令长度为0x10
- Stack 描述了发生故障的堆栈信息, 如上面例子, 堆栈顶端的 0xffffffff 是函数中的缓冲区, 随后的 0xbffffda70 是用户空间的堆栈地址
>备注: 
> 1. x86 平台上用户空间堆栈的位置小于0xc0000000
> 2. 只有 CONFIG_KALLSYMS 选项有效的情况下, Oops 信息才会有符号式的 call stack, 否则就只能看到一堆没有意义的十六进制数字

## debuger 和 相关工具

### 使用 gdb 调试内核
```sh
gdb /usr/src/linux/vmlinux /proc/kcore
```

### kdb