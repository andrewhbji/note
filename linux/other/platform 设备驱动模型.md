# Platform 设备驱动模型

## 什么是 Platform 设备

Platform 设备包括基于端口的设备(已不推荐使用，保留下来只为兼容旧设备，legacy)；连接物理总线的桥设备；集成在 SOC 平台上面的控制器；连接在其它bus上的设备（很少见）等。这些设备共同的基本特征是: 寄存器可以被直接寻址

Linux 中，驱动工程师大多需要开发 platform 设备驱动。在设备模型(总线、设备、驱动)的基础上进一步封装，抽象出 Platform Bus、 Platform Device 和 Platform Driver，供开发人员使用

## Platform 设备驱动模型的软件架构

内核中 Platform 设备有关的实现位于 include/linux/platform_device.h 和drivers/base/platform.c 两个文件中

- Platform Bus，基于底层bus模块，抽象出一个虚拟的Platform bus，用于挂载Platform设备
- Platform Device，基于底层device模块，抽象出Platform Device，用于表示Platform设备
- Platform Driver，基于底层device_driver模块，抽象出Platform Driver，用于驱动Platform设备

## Platform 设备驱动模型 API

### Platform 设备 API

platform_device 结构体(定义于 linux/platform_device.h)
```c
struct platform_device {
	const char	*name;
	int		id;
	bool		id_auto;
	struct device	dev;
	u32		num_resources;
	struct resource	*resource;

	const struct platform_device_id	*id_entry;
	char *driver_override; /* Driver name to force a match */

	/* MFD cell pointer */
	struct mfd_cell *mfd_cell;

	/* arch specific additions */
	struct pdev_archdata	archdata;
};
```
- name  设备名
- id  设备标识符
- id_auto  指示在注册设备时是否自动设置ID值，默认为 0，不自动设置
- dev  内嵌的 device 结构体
- num_resources、resource  该设备的资源描述，由struct resource（include/linux/ioport.h）结构抽象

- id_entry  和内核模块相关的内容
- mfd_cell  和MFD设备相关的内容
- archdata  保存一些architecture相关的数据

Platform 设备操作函数：

```c
static inline struct platform_device *platform_device_register_data( struct device *parent, const char *name, int id, const void *data, size_t size)
extern struct platform_device *platform_device_alloc(const char *name, int id);
extern int platform_device_add_resources(struct platform_device *pdev, const struct resource *res, unsigned int num);
extern int platform_device_add_data(struct platform_device *pdev,const void *data, size_t size);
extern int platform_device_add(struct platform_device *pdev);
extern void platform_device_del(struct platform_device *pdev);
extern void platform_device_put(struct platform_device *pdev);
```
- platform_device_register platform_device_unregister 是 Platform 设备的注册/注销接口，和 device 的 device_register 等接口类似
- arch_setup_pdev_archdata 设置platform_device变量中的archdata指针
- platform_get_resource、platform_get_irq、platform_get_resource_byname、platform_get_irq_byname，通过这些接口，可以获取platform_device变量中的resource信息，以及直接获取IRQ的number等等
- platform_device_register_full、platform_device_register_resndata、platform_device_register_simple、platform_device_register_data，其它形式的设备注册。调用者只需要提供一些必要的信息，如name、ID、resource等，Platform模块就会自动分配一个struct platform_device变量，填充内容后，注册到内核中
- platform_device_alloc，以name和id为参数，动态分配一个struct platform_device变量
- platform_device_add_resources，向platform device中增加资源描述
- platform_device_add_data，向platform device中添加自定义的数据（保存在pdev->dev.platform_data指针中）
- platform_device_add、platform_device_del platform_device_put 其它操作接口

### Platform 驱动 API

platform_driver 结构体(定义于 linux/platform_device.h)封装了 device_driver 结构体，并且需要提供 probe remove suspend resume 等回调函数
```c
struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};
```
- id_table 指针   用于在 match 函数中匹配 device 和 device_driver

Platform 驱动操作函数：

```c
extern int platform_driver_register(struct platform_driver *);
extern void platform_driver_unregister(struct platform_driver *);
extern int platform_driver_probe(struct platform_driver *driver, int (*probe)(struct platform_device *));
static inline void *platform_get_drvdata(const struct platform_device *pdev)
```
- platform_driver_registe、platform_driver_unregister，platform driver的注册、注销接口。
- platform_driver_probe，主动执行probe操作
- platform_set_drvdata、platform_get_drvdata 设置或者获取driver保存在device变量中的私有数据

platform_create_bundle 函数可在注册 platform device 的同时，根据传入参数创建并注册一个 platform driver，并返回其指针
```
extern struct platform_device *platform_create_bundle( struct platform_driver *driver, int (*probe)(struct platform_device *), struct resource *res, unsigned int n_res, const void *data, size_t size);
```
- driver res 和 probe 提供给 platform core 模块，让 platform core 模块分配资源，执行 probe 操作
- 这个函数一般用于不需要热插拔的设备

## Early platform device/driver

内核启动时，要完成一定的初始化操作之后，才会处理 device 和 driver 的注册及 probe，因此在这之前，常规的 platform 设备是无法使用的。但是在 Linux 中，有些设备需要尽早使用（如在启动过程中充当 console 输出的 serial 设备），所以 platform 模块提供了一种称作 Early platform device/driver 的机制，允许驱动开发人员，在开发驱动时，向内核注册可在内核早期启动过程中使用的driver

### Early platform device/driver API

```c
extern int early_platform_driver_register(struct early_platform_driver *epdrv, char *buf);
extern void early_platform_add_devices(struct platform_device **devs, int num);
static inline int is_early_platform_device(struct platform_device *pdev);
extern void early_platform_driver_register_all(char *class_str);
extern int early_platform_driver_probe(char *class_str, int nr_probe, int user_only);
extern void early_platform_cleanup(void);
```
- early_platform_driver_register  注册一个用于 Early device的 driver
- early_platform_add_devices  添加一个 Early device
- is_early_platform_device  判断指定的 device 是否是 Early device
- early_platform_driver_register_all  将指定 class 的所有 driver 注册为Early device driver
- early_platform_driver_probe probe 指定 class 的 Early device  
- early_platform_cleanup  清除所有的 Early device/driver

## Platform 设备驱动模型分析

Platform core 模块的初始化是由 drivers/base/platform.c 中 platform_bus_init 函数完成的，模块启动时，先注册一个名称为 platform_bus 的设备（这个操作会自动创建 /sys/devices/platform 路径），所有的 Platform 设备都将包含在其中

然后根据 platform_bus_type 注册 platform 总线（会自动创建 /sys/bus/platform/ 路径，并在其中创建 uevent drivers_probe drivers_autoprobe 等属性文件和  device driver 路径）

Platform core 模块内部使用 platform_device_add 和 platform_driver_register 函数执行 platform device 和 platform driver 的注册

platform_device_add 函数执行以下步骤：

- 如果设备没有指定父设备，将其父设备设置为 platform_bus，此时该设备在 sysfs 中的路径就是 /sys/devices/platfrom/xxx_device
- 指定设备的 bus 为 platform_bus_type
- 根据设备 ID，修改或者设置设备的名称。对于多个同名的设备，使用ID区分，在这里将实际名称修改为“name.id”的形式
- 调用 resource 模块的 insert_resource 接口，将该设备需要使用的 resource 统一管理
  >因为在这之前，只是声明了本设备需要使用哪些resource，但resource模块并不知情，也就无从管理，因此需要告知

- 调用 device_add 函数，将内嵌的 struct device 变量添加到内核中

platform_driver_register 函数执行以下步骤：

- 将 platform driver 的 bus 指定为 platform_bus_type
- 如果 platform driver 提供了probe remove shutdown 等回调函数，将该它内嵌的 struct driver 变量的 probe remove shutdown 等指针，设置为 platform 模块提供函数，包括 platform_drv_probe platform_drv_remove 和 platform_drv_shutdown
- 调用 driver_register 函数，将内嵌的 struct driver变量添加到内核中

最后通过 Linux 设备驱动模型扫描并匹配所有 platform device 和 platform driver，将 platform device 和 platform driver 关联起来
