# 第三章 字符设备

[TOC]
## 设备编号
## 文件操作
## 文件结构
## inode结构
## 注册字符设备
## open 和 release
## read write copy_to_user copy_from_user 函数

## 设备编号
字符设备可以通过文件访问,这些文件被成为特殊文件、设备文件、或简单成为文件系统的节点;这些文件通常位于 /dev/下.

使用 ls 命令的时候,字符设备文件的权限字段前用"c"标示，块设备用"b"标示,时间前面的两个数字字段分别是主设备号和次设备号,一般主设备号对应一个驱动程序，次设备号用来让内核确定设备文件所指的设备
```sh
$ ls -al /dev
crw-rw-rw-   1 root root         1,   3  Apr 11  2002 null  #5 是主设备号，3 是次设备号
brw-rw----   1 root root        10,   1  Apr 11  2002 psaux #10 是主设备号，1 是次设备号
```
>备注：现在的linux内核允许多个驱动程序共享主设备号

### MAJOR, MINOR 和 MKDEV
Linux 内核使用 dev_t (在linux/types.h中定义) 描述设备编号，dev_t 长度为32位(Linux 2.6.0),前12为是主设备号,后20位是次设备号

MAJOR 和 MAJOR 这两个宏(在linux/kdev_t.h中定义)用来获取主/次设备编号，MKDEV 用于将现有的 int 变量转换为 dev_t
```c
#ifndef _UAPI_LINUX_KDEV_T_H
#define _UAPI_LINUX_KDEV_T_H
#ifndef __KERNEL__

#define MAJOR(dev)	((dev)>>8)      //主设备号是前24位
#define MINOR(dev)	((dev) & 0xff)  //次设备是低8位
#define MKDEV(ma,mi)	((ma)<<8 | (mi))
#endif /* __KERNEL__ */
#endif /* _UAPI_LINUX_KDEV_T_H */
```

### 字符设备编号的分配和释放
```c
#include <linux/fs.h>
int register_chrdev_region(dev_t dev, unsigned int count, char* name);
int alloc_chrdev_region(dev_t *dev, unsigned firstminor, unsigned count, char* name);
返回值:成功返回0；失败返回-1
void unregister_chrdev_region(dev_t dev, unsigned int count);
```
register_chrdev_region 函数将 dev 指定的设备编号注册为字符设备编号
- count 描述要分配次设备的数量
- name 描述该字符设备的名称

alloc_chrdev_region 用来在动态分配字符设备编号
- dev_t 指针指向 alloc_chrdev_region 分配的主次设备编号
- firstminor 描述要分配的第一个次设备编号
- count 描述要分配次设备的数量
- *name 描述该字符设备的名称

unregister_chrdev_region 用来释放字符设备
- dev_t 指定要释放的主次设备编号
- count 指定次设备的数量

Linux 内核源码树中提供的设备驱动一般静态分配设备编号，这些编号在 Documentation/devices.txt描述；对于新的设备驱动，一般选择动态分配设备的方式

## 文件操作
file_operation 结构体(在linux/fs.h中定义)用来在驱动程序中注册用于文件操作的函数，同时向用户空间提供访问设备的能力
```c
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
    ...
};
```
- struct module \*owner 指向一个 module 结构体,通常被指定为 THIS_MODULE （这个宏在linux/module.h）中定义. THIS_MODULE 指向模块本身的起始地址，当file_operation中注册的文件操作正在被使用时，这个字段会阻止模块被卸载.

- loff_t (\*llseek) (struct file \*, loff_t, int); 

    设置读/写设备文件的偏移量

- ssize_t (\*read) (struct file *, char __user \*, size_t, loff_t \*); 

    读设备文件.设置为NULL会导致用户空间的 read 操作出错并返回 -EINVAL("Invalid argument") .非负的返回值代表成功读取的字节数

- ssize_t (\*write) (struct file *, const char __user \*, size_t, loff_t \*);

    写设备文件.设置为NULL会导致用户空间的 write 操作出错并返回 -EINVAL.非负返回值代表写入的字符数

- ssize_t (\*aio_write)(struct kiocb \*, const char __user \*, size_t, loff_t \*);

    异步写设备文件

- int (\*readdir) (struct file \*, void \*, filldir_t);

    读取目录.对于设备文件，这个成员应当设置为NULL

- unsigned int (\*poll) (struct file \*, struct poll_table_struct \*);

    poll 方法为 poll, epoll, 和 select 这3个系统调用提供支持， 用于查询对一个或多个文件描述符的读或写是否会阻塞. poll 方法应该返回一个代表下次读写操作是否会阻塞的位掩码；如果可能，应当提供这个信息，供内核用来将调用进程睡眠，直到可以顺利进行I/O为止. 若设置为NULL，则内核会认为这个设备永远都可以流畅的读写，而不会阻塞

- int (\*ioctl) (struct inode \*, struct file \*, unsigned int, unsigned long);
    执行设备专属的指令，例如格式化软盘上的某磁道就是专属指令，这个操作非读非写，而且进队特定设备才有意义. 用户空间程序通过使用 ioctl 系统调用来传达指令; 内核本身预先定义了少数几个不必参考 fops 表的 ioctl 通用指令.若设置内NULL，当调用 ioctl 系统调用传递内核不认识的指令，用户空间进程会收到 -ENOTTY(No such ioctl for device)
    
- int (\*mmap) (struct file \*, struct vm_area_struct \*);

    用于存储映射操作，用户空间进程通过使用 mmap 系统调用，要求驱动程序将设备上的内存映射到进程的地址空间. 若设置为NULL，调用 mmap 会返回 -ENODEV
    
- int (\*open) (struct inode \*, struct file \*);

    打开文件，如果设置为NULL，用户空间进程一定可以通过使用 open 系统调用打开文件，但是驱动程序不会收到通知

- int (\*flush) (struct file \*);

    flush 操作在用户空间进程使用 close 系统调用关闭设备文件的描述符时调用, 它应该执行并等待任何尚未完成的操作. 这货很容易和 fsync 系统调用混淆，而且目前除了SCSI磁带机等少数设备以外，很少被使用. 若设置为空，内核会直接忽略用户空间进程的关闭设备的请求
    
- int (\*release) (struct inode \*, struct file \*);
    
    释放设备文件，在 file 结构体被释放之前， release 会被触发一次. 设置为NULL，用户空间使用相关的系统调用，内核则会忽略
    
- int (\*fsync) (struct file \*, struct dentry \*, int);
    
    为 fsync 系统调用提供支持， 用来将内核高速缓冲区内的数据写入设备
    
- int (\*aio_fsync)(struct kiocb \*, int);
    
    为 aio_fsync 系统调用提供支持，设置为NULL会忽略 aio_fsync 系统调用 
    
- int (\*fasync) (int, struct file \*, int);

    和异步通知(第六章)相关，这个操作用来通知设备他的FASYNC标志已经改变，设置为NULL代表驱动不支持异步通知 
    
- int (\*lock) (struct file \*, int, struct file_lock \*);

    lock 操作用来实现文件加锁. 加锁对常规文件是必须的，但是设备驱动几乎从不实现

- ssize_t (\*readv) (struct file \*, const struct iovec \*, unsigned long, loff_t \*);
- ssize_t (\*writev) (struct file \*, const struct iovec \*, unsigned long, loff_t \*);
    
    为 readv 和 writev 系统调用提供支持，设置为NULL则忽略 readv 和 writev

- ssize_t (\*sendfile)(struct file \*, loff_t \*, size_t, read_actor_t, void \*);

    实现 sendfile 系统调用的读取逻辑，使用 DMA 将数据源(硬盘)数据读取到内核缓冲区，然后复制到输出端(如网卡)对应的内核缓冲区
    >备注：sendfile 原型是： sendfile(socket, file, len);

- ssize_t (\*sendpage) (struct file \*, struct page \*, int, size_t, loff_t \*, int);

    实现 sendfile 的输出逻辑，DMA 将输出端内核缓冲区的数据发送到对应文件或设备

- unsigned long (\*get_unmapped_area)(struct file \*, unsigned long, unsigned long, unsigned long, unsigned long);

    用于在进程的地址空间上获取一段连续的内存，这段内存在存储映射时使用

- int (\*check_flags)(int)

    这个方法允许模块检查传递给 fnctl(F_SETFL...) 调用的标志.

- int (\*dir_notify)(struct file \*, unsigned long);

    在应用程序使用 fcntl 请求目录改变通知时调用，只对文件系统有用

## 文件结构
file 结构体(在linux/fs.h中定义)代表一个打开的文件，用户空间程序调用 open 的时候，内核会创建这个结构体，然后将 file_operations 结构体的指针传递给过来， 同时内核将所有打开该文件的进程和 file 结构体关联起来(apue第三章第十节).直到所有与之关联的进程都解除关联，内核释放这个数据结构

file 结构体成员如下:
- mode_t f_mode;

    用于检查文件的读写权限, 取值如 FMODE_READ, FMODE_WRITE 等

- loff_t f_pos;
    
    用于获取文件当前的读写位置, loff_t 长度为 64 位. 除了 llseek 函数, 其他函数都不能改变 f_pos 值

- unsigned int f_flags;
    
    对应用户空间进程调用 open 函数时指定的 oflag 参数,如 O_RDONLY, O_NONBLOCK, O_SYNC等. 驱动应检查 f_flags 是否是 O_NONBLOCK, 其他的值很少被使用

- struct file_operations \*f_op;

    系统调用 open 被使用时, 内核会为这个指针赋值, 以后通过这个指针访问对应的文件操作

- void \*private_data;

    private_data 用于维护不同的系统调用都用的到状态信息

- struct dentry \*f_dentry;
    
    与该文件关联的目录项 (dentry) 结构, 一般不用关心该结构，除非使用 flip->f_dentry->d_inode 读写 inode 结构

>备注: 为避免 file 结构体和其指针混淆，一般称指向 file 结构体的指针为 filp.

## inode结构
inode 结构用来记录文件的属性，同时记录此文件的数据所在的 block 号码

inode 结构只有两个成员用于编写驱动程序：
- dev_t i_rdev;

    对于 inode结构来说, 代表设备文件, 这个成员包含实际的设备编号

- struct cdev \*i_cdev;

    当设备文件是字符设备时, i_cdev 指向一个 cdev 结构
    
## 初始化和注册字符设备
获得字符设备编号后，就可以初始化和注册字符设备了

### cdev 结构
内核使用 cdev 结构代表字符设备, 所以在内核调用字符设备前，需要创建并初始化 cdev(在 cdev.h 中定义)

```c
struct cdev {
    struct kobject kobj;
	struct module *owner;
	const struct file_operations *ops;
	struct list_head list;
	dev_t dev;
	unsigned int count;
};
```
- \*owner 用于设置实现驱动的模块, 一般为 THIS_MODULE
- \*ops 用于指定操纵这个字符设备文件的函数集合
- list 是与 cdev 对应的字符设备文件的 inode->i_devices 的链表头
- dev 是起始设备编号
- count 是设备编号范围大小

### cdev 分配和注册

1. 初始化 cdev

    cdev 初始化方法有两种：
    - cdev_malloc 函数用于自动分配一个 cdev 结构, 并返回该结构的指针. 一般使用下面的方式自动分配 cdev 结构
    ```c
    #include <cdev.h>
    struct cdev *my_cdev = cdev_alloc();
    my_cdev->ops = &my_fops;
    ```

    - cdev_init 函数用户分配指定地址的 cdev 结构， cdev 指定地址, *fops 指定文件操作
    ```c
    #include <cdev.h>
    void cdev_init(struct cdev *cdev, struct file_operations *fops);
    ```

2. 注册 cdev
    
    cdev_add 函数将 num 开始的 count 个设备注册为字符设备, 并将字符设备记录在一个 kobj_map 结构体变量的 cdev_map 成员中, cdev_map 包含一个散列表用来快速存储所有对象, 后续只要打开一个字符设备文件, 通过调用 kobj_lookup 函数, 根据设备编号就可以 cdev 结构体, 从而取出其中 ops 字段
    ```c
    #include <cdev.h>
    int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
    ```

3. 移除 cdev
    
    cdev_del 函数移除指定的字符设备, 字符设备移除后, 就不能再被读写
    ```c
    void cdev_del(struct cdev *dev);
    ```
 
示例

[scull/scull.h](../examples/scull/scull.h)
```c
struct scull_dev { 
 struct scull_qset *data;  /* Pointer to first quantum set */ 
 int quantum;  /* the current quantum size */ 
 int qset;  /* the current array size */ 
 unsigned long size;  /* amount of data stored here */ 
 unsigned int access_key;  /* used by sculluid and scullpriv */ 
 struct semaphore sem;  /* mutual exclusion semaphore  */ 

 struct cdev cdev; /* Char device structure */
};
```
[scull/main.c](../examples/scull/main.c)
```c
static void scull_setup_cdev(struct scull_dev *dev, int index)
{
 int err, devno = MKDEV(scull_major, scull_minor + index);

 cdev_init(&dev->cdev, &scull_fops);
 dev->cdev.owner = THIS_MODULE;
 dev->cdev.ops = &scull_fops;
 err = cdev_add (&dev->cdev, devno, 1);
 /* Fail gracefully if need be */
 if (err)
 printk(KERN_NOTICE "Error %d adding scull%d", err, index);
} 
```

## open 和 release

### open 函数
```c
int (*open)(struct inode *inode, struct file *filp);
```

open 函数在实现的时候，应当执行下面的操作
- 获取当前被开启的设备，如果目标设备是首次被开启，则应初始化设备
- 检查设备特有的错误(设备没有就绪等任何硬件方面的问题)
- 如果需要的话，更新 f_op 指针
- 分配并装填任何需要放在 file->private_data 的数据结构

open 函数传递进来的 \*inode->i_cdev 包含当前被打开设备的 cdev 结构, 为便于快速的操作 cdev (如判断目标设备是否首次被开启), Linux 提供 container_of 宏(在linux/kernel.h中定义), 可以根据 open 函数传递进来的 \*inode 找到当前被打开的 cdev 所属的数据结构
```c
container_of(pointer, container_type, container_field); 
```
container_of 需要三个参数：pointer 指向一个名为 container_field 的元素, 该元素所属的结构体由 container_type 指定. 这个宏会返回 container_field 元素所属的结构体的指针, 如下面代码

[scull/scull.h](../examples/scull/scull.h)
```c
struct scull_dev {
	struct scull_qset *data;  /* Pointer to first quantum set */
	int quantum;              /* the current quantum size */
	int qset;                 /* the current array size */
	unsigned long size;       /* amount of data stored here */
	unsigned int access_key;  /* used by sculluid and scullpriv */
	struct semaphore sem;     /* mutual exclusion semaphore     */
	struct cdev cdev;	  /* Char device structure		*/
};
```
[scull/main.c](../examples/scull/main.c)
```c
struct scull_dev *dev;
dev = container_of(inode->i_cdev, struct scull_dev, cdev);
```

并不是所有 file_operations 操作的实现都会传入 inode, 所以可以将 container_of 获取到 dev 保存在 filp->private_data 中, 供其他 file_operations 使用

### release 函数
release 函数在实现的时候，应当执行下面的操作
- 释放存在与 filp->private_data 的数据
- 在最后一次关闭时, 将目标设备关机

并非每一次使用 close 系统调用都会触发 release, 只有使用 close 时 file 结构体内维护的文件开启次数元素归零, 才会触发 release

## kmalloc 和 kfree
这两个函数用于管理内核空间的内存
```c
#include <linux/slab.h>
void *kmalloc(size_t size, int flags);
void kfree(void *ptr);
```
- flags 用于描述如何分配内存, 一般设置为GFP_KERNEL

## read write copy_to_user copy_from_user 函数
```c
ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
返回值: 成功返回实际读写的字节数;失败返回负数
```
- \*filp 指向 file 结构体
- 对于 read 函数, \*buff 指向用户空间的内存, 用于临时存放从设备读取出来的数据; 对于 write 函数, \*buff 指向用户空间的内存， 用于存放即将写入设备的数据
- count 指出需要读写的字节数
- \*offp 指定读写的偏移量

内核空间不能直接访问指向用户空间的指针, 需要使用 asm/uaccess.h 定义的两个函数
```c
#include <asm/uaccess.h>
unsigned long copy_to_user(void __user *to,const void *from,unsigned long count); 
unsigned long copy_from_user(void *to,const void __user *from,unsigned long count); 
返回值: 成功返回0; 失败返回没有成功拷贝的字节数; 出错返回负值
```
- count 指示拷贝的字节数
- \*from 、 \*to 指向的地址无效, 或拷贝过程中遇到无效地址都会导致拷贝失败
- 如果出错, 返回值一般使用 <linux/errno.h> 中定义的错误码

## readv 和 writev 函数
readv 和 writev 函数都可以包含多块缓冲区, readv 会将从设备读取出的数据依次写到这些缓冲区, writev 会将所有缓冲区中的数据一次性写入到设备. 相对于 read 和 write, 这两个函数更为高效
```c
ssize_t (*readv) (struct file *filp, const struct iovec *iov, unsigned long count, loff_t *ppos);
ssize_t (*writev) (struct file *filp, const struct iovec *iov, unsigned long count, loff_t *ppos);
```
- 如果这两个函数没有实现, 那么 readv 和 writev 系统调用会通过多次调用 read 和 write 函数来实现
- \*iov 是 iovec 结构数组
- iovec 结构, 定义于 <linux/uio.h>, 每一个 iovec 结构都描述了一块被用于读写数据的缓冲区, iov_base 指向该缓冲区的首地址, iov_len 指定这块缓冲区有多少个字节
```c
struct iovec
{
    void __user *iov_base;
    __kernel_size_t iov_len;
};
```
- count 指定需要读写多少个 iovec 结构
