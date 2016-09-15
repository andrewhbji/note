# 第十二章 PCI(外部设备互联总线) 驱动程序

## PCI 寻址方法

每一个 PCI 设备都有一个专属地址, 这个地址由总线编号、设备编号和功能编号组成

Linux 所支持的每个 PCI 域(16-bit)最多可容纳256个总线(8-bit)，每个总线最多可容纳32个设备(5-bit)，每个设备最多可以由8项功能(3-bit)

在硬件层级上，每一个功能都可以用一个32位的地址来表示, 该地址被成为PCI地址, 或成为索引(key)

Linux 内核使用 pci_dev 结构体(定义于linux/pci.h)来标识 PCI 设备

硬件方面:

- 每一个 PCI 设备的电路都要能提供其内存的位置、I/O 端口、组态寄存器
- PCI 总线上的所有设备共用内存空间和I/O空间, 当某个地址的内存被访问时, 相同总线上的全部设备会同时收到相同的地址信号, 所以 PCI 总线上的不同设备, 彼此的内存区必须分开
- 组态寄存器的地址采用位置式寻址法, 这表示地址信号一次只会出现在一个插槽, 所以 PCI 设备的组态寄存器可以使用相同的地址, 而不发生冲突

驱动程序方面:

- 访问 PCI 设备上的内存和 I/O 端口, 可以使用 inb readb 等函数, 访问组态寄存器必须使用内核提供的函数
- PCI 设备最多可以使用四个不同的 IRQ, 不同的 PCI 设备可以共享 IRQ


内存和I/O空间方面:
- PCI 的 I/O 空间(32-bit 地址总线)和内存空间(32 或 64-bit 地址总线)是分离的
- PCI 规格的设备上每一块内存区与I/O区都必须可被映射到其他的地址范围, 系统开机时,PCI BIOS 的固件会负责协调、映射各PCI设备的地址空间, 并逐一初始化每一个PCI设备, 重新映射各设备的内存区与I/O区, 使所有区域分开
- 系统软件可以从组态空间取得PIC设备的对应地址区, 并使用新的对应地址来访问PCI设备
-  PCI 设备的驱动程序不需要自己探测硬件, 只需要读入组态寄存器的内容, 就可以安全的访问硬件

设备的每个功能, 在 PCI 组态空间中各占 256 个字节(PCI Express 设备占4k 字节), 组态寄存器的配置和布局有固定的标准, 其中有 4 字节被成为 function ID, 可供驱动程序用来识别其所要驱动的目标设备

位置式寻址法让 PCI BIOS 找出各 PCI 设备的组态空间, 然后通过这些组态寄存器提供的信息, 驱动程序就可以对 PCI 设备实施一般的 I/O 访问, 而不需要进行定位

## 开机自检

PCI 设备通电后, 硬件设备还没有激活, 只对组态交涉有反应, 此时设备上的内存和I/O端口都没有映射到主机的地址空间, 所有辅助工作(中断、DMA)和主要工作(显示、控制磁盘)都不能进行

PCI BIOS 通过访问PCI控制器上的寄存器提供访问组态空间的能力

系统启动时, PCI BIOS 或 Linux 内核会逐一对每一个 PCI 设备进行组态交涉, 然后映射各PCI设备的内存区和I/O区到CPU的地址空间(驱动程序没必要修改预设的映射方式)

## 组态寄存器和初始化

PCI 设备的至少一个有256个字节的地址空间, 前64字节是标准的(如图所示), 剩下的由设备决定

![../img/ldd3-12-2.png](标准 PCI 组态寄存器的布局)

必选寄存器和可选寄存器:
- 必选寄存器用于存放PCI设备必须提供的信息, 如 vendor ID
- 可选寄存器由设备的实际功能确定
- 必选寄存器的另一个功能是声明哪些可选寄存器是否可用

常用组态寄存器介绍:

- vendorID: 16-bit, 标识设备制造商, 如 Intel 的设备被标识为0x8086
- deviceID: 16-bit, 由设备制造商提供, 和 vendorID 一起标识一个产品
- class: 16-bit, 标识设备的分类, 其高8位标识设备所属的大类, 如 network, communication等
- subsystem vendorID, subsystem deviceID: 用于标识子系统的 vendorID 和 deviceID 

pic_device_id 结构体:

pci_device_id 结构体(linux/mod_devicetable.h)可以被用来定义一个列表, 用来包含驱动程序所支持的不同的设备
```c
struct pci_device_id {
	__u32 vendor, device;		/* Vendor and device ID or PCI_ANY_ID*/
	__u32 subvendor, subdevice;	/* Subsystem ID's or PCI_ANY_ID */
	__u32 class, class_mask;	/* (class,subclass,prog-if) triplet */
	kernel_ulong_t driver_data;	/* Data private to the driver */
};
```
- vendor, device 参数用来指定vendorID和deviceID
- subvendor,subdevice 参数用来指定subsystem vendorID 和 subsystem deviceID
- class, class_mask 指定驱动程序所支持的某一类或某些类设备, 如果一个驱动可处理任何子系统(比如芯片供应商vendor为intel, 设备制造商subvendor包括asus dell 等) , 可以指定为 PCI_ANY_ID
- driver_data 用来指定一些其他信息, PCI 驱动可以用来区分不同的设备

PCI_DEVICE 宏用来初始化一个 pci_device_id 结构体
```c
PCI_DEVICE(vendor, device)
PCI_DEVICE_CLASS(device_class, device_class_mask)
```
- PCI_DEVICE 宏可通过 vendor, device 来初始化 pci_device_id 结构体
- PCI_DEVICE_CLASS 宏可通过 device_class, device_class_mask 来初始化 pci_device_id 结构体

## MODULE_DEVICE_TABLE 宏

pci_device_id 结构体必须释出给 user-space, 让热插拔与模块加载系统知道硬件和驱动程序的对应关系

MODULE_DEVICE_TABLE 宏(定义于linux/module.h)用于释出 pci_device_id 结构体数组
```c
MODULE_DEVICE_TABLE(type,pci_device_id_array)
```
- type 描述设备类型, 取值为 pci usb 等

MODULE_DEVICE_TABLE 宏会建立一个 __mod_pci_device_table 变量, 指向 pci_device_id 数组. 在构建内核的时候, depmod 工具会找出所有含有 __mod_pci_device_table 的模块, 然后将模块里的数据取出并保存到 /lib/modules/KERNEL_VERSION/modules.pcimap 文件中. 在 depmod 完成后, 就可从该文件中获知当前版本内核的所有PCI驱动程序所支持的PCI设备, 以及对应的驱动程序模块名称. 内核运行期间, 如果发现新的 PCI 设备, 就可对热插拔系统发出通知, 这时热插拔系统就可从modules.pcimap 文件找出应该载入的驱动程序

## 注册PCI驱动程序

pci_driver 结构体(定义于linux/pci.h)用来描述PCI驱动程序
```c
struct pci_driver {
	const char *name;
	const struct pci_device_id *id_table;	/* must be non-NULL for probe to be called */
	int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);	/* New device inserted */
	void (*remove) (struct pci_dev *dev);	/* Device removed (NULL if not a hot-plug capable driver) */
	int  (*suspend) (struct pci_dev *dev, pm_message_t state);	/* Device suspended */
	int  (*resume) (struct pci_dev *dev);	                /* Device woken up */
};
```
- name 描述驱动程序的名称, 当驱动被嵌入内核时, 此名称会出现在/sys/bus/pci/drivers 中
- id_table 指向 pci_device_id 结构体数组指针
- probe 指向PCI驱动程序的探测函数, 当内核认为它找到一个应该由驱动程序控制的 pci_dev 结构体时, 会调用这个函数, 并将该 pci_device_id 结构体数组的指针也传递进来让驱动程序进行检查. 如果 pci_dev 结构体应由当前驱动程序控制, 则需要初始化硬件设备, 并返回0, 如果不是, 或发生错误, 则返回一个负的错误码
- remove 指向 PCI 驱动程序的移除函数, 如果内核发现 struct pci_dev 要被移除出系统, 或 PCI 驱动程序被卸载时, 自动调用此函数
- suspend 指向 PCI 驱动程序的搁置函数, 当内核发现 struct pci_dev 要被搁置时, 会呼叫此函数, 搁置函数可从它的 state 参数得到搁置状态, 此函数不一定要提供
- 指向恢复函数, 当内核发现 struct pci_dev 要从搁置状态恢复时, 会调用次函数

pci_register_driver 用于在模块初始化期间注册PCI驱动程序
```c
static inline int pci_register_driver(struct pci_driver *drv)
```
- 返回 0 代表成功, 返回负数代表失败

pci_unregister_driver 用于注销PCI驱动
```c
static inline void pci_unregister_driver(struct pci_driver *drv)
```

## 启用PCI设备

PCI驱动程序的 probe 函数中, 在访问设备的任何资源(I/O区或中断)前, 需要确保已启用PCI 设备

pci_enable_device 函数(定义于linux/pci.h)用来启用PIC设备
```c
static inline int pci_enable_device(struct pci_dev *dev)
```

## 访问组态寄存器

驱动程序通常需要访问三种地址空间: 内存 I/O端口 组态寄存器

CPU 不能直接访问组态寄存器, 一般通过PCI控制器来访问

linux 内核提供了访问PCI组态寄存器的标准接口
```c
int pci_read_config_byte(struct pci_dev *dev, int where, u8 *val);
int pci_read_config_word(struct pci_dev *dev, int where, u16 *val);
int pci_read_config_dword(struct pci_dev *dev, int where, u32 *val);

int pci_write_config_byte(struct pci_dev *dev, int where, u8 val);
int pci_write_config_word(struct pci_dev *dev, int where, u16 val);
int pci_write_config_dword(struct pci_dev *dev, int where, u32 val);
```
- 这些函数实际上真正调用的是pci_bus_write_config_* 和 pci_bus_read_config_*
- where 参数用于指定要访问的组态寄存器, 常见的值定义在 linux/pci_regs.h中

## 访问 I/O 和 内存空间

PCI 设备最多可以有六段 I/O 地址区, 其大小与实际映射位置分别定义在PCI_BASE_ADDRESS_0--PCI_BASE_ADDRESS_5这六个32-bit组态寄存器中; 如果PCI 的内存空间使用64-bit地址总线, 则使用两个相邻的PCI_BASE_ADDRESS组态寄存器来描述其大小和实际映射位置

linux 内核提供 pci_resource_* 函数用来获取 PCI_BASE_ADDRESS 组态寄存器所描述的I/O 地址区信息
```c
unsigned long pci_resource_start(struct pci_dev *dev, int bar);
unsigned long pci_resource_end(struct pci_dev *dev, int bar);
unsigned long pci_resource_flags(struct pci_dev *dev, int bar);
```
- pci_resource_start 会返回 bar 指定的组态寄存器所描述PCI I/O地址区的首地址 
- pci_resource_end 会返回 bar 指定的组态寄存器所描述PCI I/O地址区的末地址 
- pci_resource_flags 会返回 bar 指定的组态寄存器所描述PCI I/O地址区的状态标志位, 常见的取值(定义于linux/ioport.h)包括
    - IORESOURCE_IO I/O     地址区为I/O地址
    - IORESOURCE_MEM I/O    地址区为内存地址
    - IORESOURCE_PREFETCH   地址区可以被缓存
    - IORESOURCE_READONLY   地址区只读

## PCI 中断

Linux 内核加载之前, PCI BIOS 已经分配好各设备的 IRQ 通道, 驱动程序可以直接使用

IRQ 通道记录在 PCI_INTERRUPT_LINE 组态寄存器(8-bit)中, 该寄存器最多可以记录256条中断线, 访问该寄存器可以获取IRQ通道编号, 然后就可以在中断处理中使用, 而不必额外的去探测IRQ通道

PCI_INTERRUPT_PIN 组态寄存器(8-bit)用来记录设备是否支持中断, 值为0表示不支持, 非0值表示支持

这两个组态寄存器可通过 pci_read_config_byte来访问

## 硬件抽象层

linux 内核实际上通过 pci_ops 结构体(定义于linux/pci.h)来访问组态寄存器
```c
struct pci_ops
{
        int (*read)(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val);
        int (*write)(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val);
};
```
比如 pci_read_config_byte(dev,where,val) 函数, 具体实现其实为：dev->bus->ops->read(bus,devfn,where,8,val)