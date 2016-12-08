# 第二章 建立和运行模块
Linux设备驱动的开发, 无论是USB驱动, 或是PCI设备驱动, 都利用的是Linux模块编程.

[TOC]
## 模块的初始化和清理
## 编译模块
## 内核符号表
## 测试模块
## 模块参数
## Linux 用户空间和内核空间

## 模块的初始化和清理

### module_init module_exit MODULE_LICENSE
内核提供 module_init 和 module_exit 两个宏定义函数用来分别指定模块的入口函数和卸载函数
- module_init 用于指定加载模块时必须的初始化操作
- module_exit 用于指定卸载模块时必须清理工作

MODULE_LICENSE 用来描述模块代码使用的许可,类似的，还可以使用这些宏来描述模块代码：
- MODULE_AUTHOR ( 声明谁编写了模块 )
- MODULE_DESCRIPION( 一个人可读的关于模块做什么的声明 )
- MODULE_VERSION ( 一个代码修订版本号; 版本字串使用的惯例参考 <linux/module.h> 的注释)
- MODULE_ALIAS ( 模块的别名 )
- MODULE_DEVICE_TABLE ( 告知用户空间, 模块支持那些设备 )

这个例子展示了Linux模块编程的基本方式

[hello.c](../examples/misc-modules/hello.c)

```c
#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void){
       printk(KERN_ALERT "Hello, world\n");
       return 0;
}
static void hello_exit(void){

       printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);
```

### __init  __initdata __exit __exitdata 

一般 module_init 和 module_exit 指定的函数声明的时候需要使用 __init 和 __exit 标志，这表示这个函数位于二进制文件的.init.text 或 .exit.text 文本段

__initdata __exitdata 用来标记需要初始化和需要释放的变量，分别位于 .init.data 和 .exit.text 数据段

.init.text 和 .init.data 有可能在初始化完成后被丢弃；如果模块直接内建在内核中，或内核不允许卸载模块，.exit.text 和 .exit.text 将不存在。

### 初始化过程中的错误处理
- 注册设备时通过检查返回值检查操作是否成功
- 如果注册设备失败，首先是决定模块是否能够无论如何继续执行初始化，一般选择继续初始化，并提供替代功能，否则需要取消注册，并释放内存

错误处理一般使用 goto 语句,如下面的代码
```c
int __init my_init_function(void)
{
        int err;
        /* registration takes a pointer and a name */
        err = register_this(ptr1, "skull");
        if (err)
                goto fail_this;
        err = register_that(ptr2, "skull");
        if (err)
                goto fail_that;
        err = register_those(ptr3, "skull");
        if (err)
                goto fail_those;
        return 0; /* success */
fail_those:
        unregister_that(ptr2, "skull");
fail_that:
        unregister_this(ptr1, "skull");
fail_this:
        return err; /* propagate the error */

}
```
- 也可以在出现任何错误的情况下调用 __exit 函数执行清理全部工作，但者需要更多的代码和更多的CPU时间
>备注: my_init_function 要返回错误码时，Linux内核中的自定义的错误码必须是负数, 可以对<linux/errno.h>取负数

### 模块加载竞争
- 内核的某些别的部分会在注册完成之后马上使用任何你注册的设备，也就是说内核会在模块的初始化函数还在调用的时候就已经使用模块了。所以必须在设备注册完成之前在完成代码的初始化
- 需要考虑好初始化中的错误处理，并在这些出错处理中考虑任何可能的内核操作

## 编译模块
Linux 模块的构建依赖Linux 内核构建环境，执行 make 命令的时候，需要指定内核源码树路径和模块生成的路径，由 make命令 和内核源码树执行编译，然后在指定的路径生成模块文件

### Makefile 文件
下面的Makefile表示使用hello.c建立模块hello.ko
```
obj-m := hello.o
```

### 使用Make编译模块以及编译过程
执行make命令
``` sh
make -C /usr/src/linux-2.6.10 M=`pwd` modules
```
命令说明:
- -C指定Linux内核源码树的路径
这个路径下Linux内核顶层的Makefile将被作为整个内核构建系统的入口
- M指定module目标的生成路径
M=后面是反引号(ESC键下面)而不是单引号, 里面的pwd表示把pwd命令执行的结果(即hello.c所在路径)赋值给M

执行这个命令,  可以看到生成了hello.ko, 即hello模块

问题是, 我仅仅指定了linux内核的路径以及输出路径.那么是什么帮我完成了全部的工作?

我们再看看命令输出的日志
``` sh
$ make -C /usr/src/linux-2.6.10 M=`pwd` modules
make[1]: Entering directory `/usr/src/linux-2.6.10'
 CC [M] /home/ldd3/src/misc-modules/hello.o
 Building modules,  stage 2.
 MODPOST
 CC /home/ldd3/src/misc-modules/hello.mod.o
 LD [M] /home/ldd3/src/misc-modules/hello.ko 
make[1]: Leaving directory `/usr/src/linux-2.6.10'
```
再看一遍LDD3 ch02 s4 ,  发现有这么一句: "是内核构建系统处理了余下的工作", 好像明白了些什么...

在往后看, 找到了对其全过程的描述, 
…… 
obj-m := hello.o表明有一个模块要根据目标文件hello.o建立
……
makefile, 它需要在更大的内核构建系统的环境中被调用
……
这条命令开始是改变他的目录到用-C选项提供的目录下, 在那里会发现内核的顶层makefile, M=选项使makefile在试图建立目标前, 回到模块源码目录

### 完善Makefile文件
仅仅 make hello.c这个模块还好,  但是以后还要make更多的模块,  需要一遍一遍的复制粘贴makefile, 和这个复杂的make命令吗?答案当然是: no
LDD3的作者为我们提供了一个比较好的Makefile模板:
[Makefile](../examples/misc-modules/Makefile)
```sh

# To build modules outside of the kernel tree,  we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.
ifeq ($(KERNELRELEASE), )

	# Assume the source tree is where the running kernel was built
	# You should set KERNELDIR in the environment if it's elsewhere
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	# The current directory is passed to sub-makes as argument
	PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

.PHONY: modules modules_install clean

else
	# called from kernel build system: just declare what our modules are
	obj-m := hello.o
endif
```
这个makefile提供这几个功能:
- make 或者 make modules: 编译 obj-m指定的模块
- make modules_install: 编译并安装模块
- make clean: 清理上面两条命令的执行执行后的临时文件. 如果需要清理make modules_install,  需要su权限
- make .PHONY:执行一次make modules_install,  然后清理临时文件

### 版本依赖
模块代码一定要为每个被使用的内核重新编译。模块构建的过程中其中一步就是通过使用当前内核树中的文件vermagic.o连接模块，以获得很多和构建系统相关的信息，包括目标内核版本, 编译器版本等。加载一个模块, 这些信息被检查与运行内核的兼容性. 如果不匹配, 模块不会加载。

如果需要编写在多个内核版本上运行的模块，需要在#ifdef中使用下 LINUX_VERSION_CODE、UTS_RELEASE 、KERNEL_VERSION宏定义来保证模块被正确的构建：

### 硬件平台依赖
每一个处理器平台都有其特点，内核设计者可以使用这些特点获得更好的性能

## 内核符号表
从2.6开始,必须使用EXPORT_SYMBOL将非static函数和变量导入内核空间的内核符号表，供内核中的其他模块使用，如下面的代码，mod1将func1函数导出到内核符号表，mod1加载后，mod2可以使用mod1的func1函数
[mod1.c](../examples/misc-modules/mod1.c)
``` C
#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>

static int func1(void)
{
        printk(KERN_ALERT "In Func: %s...\n",__func__);
        return 0;
}

EXPORT_SYMBOL(func1);

static int __init hello_init(void)
{
        printk(KERN_ALERT "Module 1,Init!\n");
        return 0;
}

static void __exit hello_exit(void)
{
        printk(KERN_ALERT "Module 1,Exit!\n");
}

module_init(hello_init);
module_exit(hello_exit);
```
[mod2.c](../examples/misc-modules/mod2.c)
```c
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>

static int func2(void)
{
        extern int func1(void);
        func1();
        printk(KERN_ALERT "In Func: %s...\n",__func__);
        return 0;
}

static int __init hello_init(void)
{
        printk(KERN_ALERT "Module 2,Init!\n");
        func2();
        return 0;
}

static void __exit hello_exit(void)
{
        printk(KERN_ALERT "Module 2,Exit!\n");
}

module_init(hello_init);
module_exit(hello_exit);
```
编译/加载/卸载
```shell
make modules
insmod mod1.ko
insmod mod2.ko
rmmod mod2
rmmod mod1
```

输出
```shell
[  306.992716] Module 1,Init!
[  311.645936] Module 2,Init!
[  311.645968] In Func: func1...
[  311.645983] In Func: func2...
[  321.452650] Module 2,Exit!
[  326.679747] Module 1,Exit!
```
>备注
>- printk 中必须加入 KERN_ALERT, 否则看不到输出
>- rmmod 时,如果先rmmod mod1 会看到 mod1 is in use
>- EXPORT_SYMBOL 和 System.map的区别:
>	- System.map 是链接时的函数地址,连接完成后,在内核运行过程中,是不知道那个符号在哪个地址的. 而这个文件是给调试使用的,其中的内容, kernel并不知道
>	- EXPORT_SYMBOL的符号,是把这些符号和对应的地址保存起来,在内核运行的过程中,可以找到这些符号对应的地址
>	- module在加载过程中,其本质是动态链接到内核. 如果在模块中引用了内核或其他模块的符号, 就要在内核和其他模块中EXPORT_SYMBOL这些符号, 这样才能找到对应的地址链接. 这就解释了为什么要先加载mod1

## 测试模块
模块建立后, 下一步是加载到内核, 测试模块的加载/卸载需要su权限

### 加载模块
安装模块使用insmod命令,  以hello.ko为例:
```shell
sudo insmod hello.ko
```
- 这个程序加载模块的代码段和数据段到内核, 接着,  执行一个类似 ld 的函数,  它连接模块中任何未解决的符号连接到内核的符号表上. 但是不象连接器,  内核不修改模块的磁盘文件,  而是内存内的拷贝.
- 另外一个工具: modprobe
modprobe和 insmode功能相同,  区别在于 modprobe会查看要加载的模块, 看它是否引用了当前内核中没有定义的符号. modprobe 在定义相关符号的当前模块搜索路径中寻找其他模块. 当 modprobe 找到这些模块( 要加载模块需要的 ),  它也把它们加载到内核. 如果你在这种情况下代替以使用 insmod,  命令会失败,  在系统日志文件中留下一条"nresolved symbols "消息. 

### 卸载模块
卸载模块使用rmmod命令, 以hello.ko为例:
```shell
sudo rmmod hello
```
注意,  如果内核认为模块还在用( 就是说,  一个程序仍然有一个打开文件对应模块输出的设备 ),  或者内核被配置成不允许模块去除,  模块去除会失败. 可以配置内核允许"强行"去除模块,  甚至在它们看来是忙的. 如果你到了需要这选项的地步,  但是,  事情可能已经错的太严重以至于最好的动作就是重启了. 

### 查看模块列表
```shell
lsmod
```
lsmod 程序生成一个内核中当前加载的模块的列表, 以及一些其他信息, 例如使用了一个特定模块的其他模块. lsmod 通过读取 /proc/modules 虚拟文件工作. 当前加载的模块的信息也可在位于 /sys/module 的 sysfs 虚拟文件系统找到. 

### 查看输出
使用dmsg查看输出日志
```shell
desg
```
可以看到，除了刚才安装模块时hello_init打印的”Hello, world”，又多了一条语句”Goodbye, cruel world”，这句话就是模块卸载函数hello_exit函数打印的.

## 模块参数
Linux 模块在加载的时候可以指定参数

### module_param 宏
module_param 宏用于接收模块参数，并赋值给变量
```c
#include <linux/moduleparam.h>
#define module_param(name, type, perm)
```
- name 指定变量名
- type 指定变量类型
- perm 指定模块参数的访问权限，指定了 perm 的模块参数同时也会出现再 /sys/modules/ 下，这时 perm 用来指定模块参数的访问权限；include/linux/stat.h 中定义的perm值如下：

|  perm  |  描述  |
|:--|:--|
| S_ISUID       | 设置SUID权限(执行时设置UID)       |
| S_ISGID       | 设置SGID权限(执行时设置GID)       |
| S_ISVTX       | 设置黏着位       |
| S_IRWXU       | 所有者RWX       |
| S_IRUSR       | 所有者R       |
| S_IWUSR       | 所有者W       |
| S_IXUSR       | 所有者X       |
| S_IRWXG       | group RWX       |
| S_IRGRP       | group R       |
| S_IWGRP       | group W       |
| S_IXGRP       | group X       |
| S_IRWXO       | other RWX       |
| S_IROTH       | other R       |
| S_IWOTH       | other W       |
| S_IXOTH       | other X       |
| S_IRWXUGO  | S_IRWXU \| S_IRWXG \| S_IRWXO  |
| S_IALLUGO  | S_ISUID \| S_ISGID \| S_ISVTX \| S_IRWXUGO  |
| S_IRUGO  | S_IRUSR \| S_IRGRP \| S_IROTH  |
| S_IWUGO  | S_IWUSR \| S_IWGRP \| S_IWOTH  |
| S_IXUGO  | S_IXUSR \| S_IXGRP \| S_IXOTH  |
>备注：也可以自由组合这些值的低3位来指定权限，如 module_param(irq,int,0664);

如下面的代码，char 指针 whom 是字符串类型的模块参数, howmany 是 int 类型的模块参数，在使用insmod或者modprobe加载模块的时候, 可以指定参数, 如:
insmod hellop.ko howmany=10 whom="mom"，变量就传递给模块
[hellop.c](../examples/misc-modules/hellop.c)
```c                                                
static char *whom = "world";
static int howmany = 1;

module_param(howmany, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);

static int hello_init(void)
{
	int i;
	for (i = 0; i < howmany; i++)
		printk(KERN_ALERT "(%d) Hello, %s\n", i, whom);
	return 0;
}
```

### 模块参数的类型
- bool
- invbool 一个布尔型( true 或者 false)值(相关的变量应当是 int 类型). invbool 类型颠倒了值, 所以真值变成 false, 反之亦然. 
- charp 一个字符指针值, 内存为用户提供的字符串分配, 指针因此设置
- int
- long
- short
- uint
- ulong
- ushort
- 数组

### module_param_array 和数组类型的模块参数
module_param_array 宏用于接收数组类型的模块参数
```c
#include <linux/moduleparam.h>
#define module_param_array(name, type, nump, perm)
```
- name 指定数组变量名
- type 指定数组类型
- nump 指定一个int型变量用于接收传入的数组元素个数
- perm 指定模块参数的访问权限

如下面代码，将 array 指定为int类型数组模块参数，访问权限设置为所有人可读；narr 用来接收传入的数组元素个数；当时用 insmod hellopa.ko array=1,2,3,4,5 加载模块时，narr 会被赋值为 5

[hellopa.c](../examples/misc-modules/hellopa.c)
```c
#define ARRAYSIZE 10
static int array[ARRAYSIZE];
int narr;

module_param_array(array,int,&narr,S_IRUGO);

static int hello_init(void)
{
	int i;
	for (i = 0; i < narr; i++)
		printk(KERN_ALERT "array[%d] = %d\n", i, array[i]);
	return 0;
}
```

## Linux 用户空间和内核空间
现代的CPU在执行任务的时候至少由两个权限级别，有些CPU甚至更多，UNIX系统的设计利用了这个系统特性，内核空间运行的程序具有最高级别权限，而用户空间程序则只有最低级别权限

### 内核的并发处理
用户空间的程序大多数都是顺序执行的，多线程的程序是例外，内核空间的程序则不是这样，内核程序需要在同一时间处理多个并发。所以内核程序代码必须是可重入的，相关数据结构需要考虑对并发进行同步处理，并且小心读写共享的数据

### 内核引用当前进程
内核程序可以通过读取全局项 current (在<asm/current.h>定义)来产生一个 task_struct 结构体指针(在<linux/sched.h>定义)来获取访问内核的进程(如使用系统调用的进程)的信息

### 用户空间驱动程序和内核空间驱动程序
用户空间的程序也可以驱动硬件(比如libusb)，对于在内核空间的编写驱动和用户空间编写驱动要区别对待

#### 用户空间编程的优点
- 完整的 C 库可以连接
- 相对内核空间，运行在用户空间的程序比较容易调试
- 用户空间的程序如果故障(比如锁死)，可以简单的kill掉
- 用户空间的程序是可以被交换到交换分区的，在内核空间中一个常用但是很大的驱动程序在加载后，其占用的内存不能被其他程序使用
- 用户空间的程序不需要开源
- 在用户空间的程序可以并行的读写硬件

#### 用户空间编程的缺点
- 中断在用户空间无法使用
- 只能通过存储映射 /dev/mem 来使用DMA(直接内存访问)，而且需要root权限
- 读写I/O端口只能通过调用ioperm和iopl系统调用，而且不是所有平台都支持；读写/dev/port效率有太低；这些系统调用和设备文件的操作都需要root权限
- 响应时间太慢
- 如果程序被交换到了交换分区，读写操作就会被阻塞，或者失败；使用mlock锁定内存可能会有帮助，但是不能锁定太多内存
- 块设备、网络接口等不能被用户空间程序处理

>备注：用户空间的驱动一般实现一个系统服务，它会接管(不是取代，原著这段好难懂，其实理解为中继最合适)内核，作为一个单一代理负责控制硬件的任务

### 其他
- 内核空间堆栈很小，所以尽量动态分配内存
- 内核API中以双下划线(__)开始的函数一般是底层的接口，应小心使用
- 内核代码没有必要包含浮点, 就算CPU架构允许，额外的负担不值得
