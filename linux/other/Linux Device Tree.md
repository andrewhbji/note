# Device Tree

## 引言

阅读linux内核代码就像欣赏冰山，有看得到的美景（各种内核机制及其代码），也有埋在水面之下看不到的基础（机制背后的源由和目的）。沉醉于各种内核机制的代码固然有无限乐趣，但更重要的是注入更多的思考，思考其背后的机理，真正理解软件抽象。这样才能举一反三，并应用在具体的工作和生活中。

## 缘起

### 没有Device Tree的ARM linux是如何运转的？

首先了解 Linux 2.X 内核是怎样移植的：

1. bootloader需要实现以下功能：

    - 设置和初始化 RAM
    - 初始化一个串口（可选）
    - 检测机器类型，并将 MACH_TYPE_XX 传入内核
    - 一个系统内存的位置和最小值，根文件系统位置，以设置内核标签列表（以 ATAG_CORE 开始，size 域设为0x00000002；以 ATAG_NONE 结尾， size 域必须设置为零；一般放在 RAM 的头 16KiB中）
    - 引导内核启动
    
2. 在 /arch/arm 下建立 mach-xxx 目录，放入驱动 SOC 的代码，比如 interrupt-controller timer mem-mapping sleeping audio video 等，然后建立一个board-xxx.c 文件

board-xxx.c的内容如下：
    
    - 使用 MACHINE_START 和 MACHINE_END 定义 machine
    ```c
    MACHINE_START(MACH_TYPE_XX, "machine description")
        .phys_io    = 0x40000000,
        .boot_params    = 0xa0000100,  
        .io_pg_offst    = (io_p2v(0x40000000) >> 18) & 0xfffc,
        .map_io        = xxx_map_io,
        .init_irq    = xxx_init_irq,
        .timer        = &xxx_timer,
        .init_machine    = xxx_init,
    MACHINE_END 
    ```
    - machine 包含的各种 init 函数用于初始化 SOC 设备，因此伴随 platform device 的创建会伴随大量用来描述设备信息的静态table
    
3. 调试驱动和串口terminal

### 混乱的 ARM architecture 代码和存在的问题

在 linux kernel 中支持一个 基于 ARM 的 SOC 平台是相对容易的，可以这样的实现造成了一个问题：
    arch/arm/plat-xxx和arch/arm/mach-xxx中充斥着大量的类似的代码，这些代码只是在描述板级细节，而且这些代码对内核来讲没有任何意义，如每个SOC都有功能相似的的 platform_device、resource、i2c_board_info、spi_board_info 以及各种硬件的 platform_data。
    
终于，Linus 在收到 OMAP development tree 一次提交请求邮件后再，再也不忍直视，并发出了怒吼：

    Gaah. Guys, this whole ARM thing is a f*cking pain in the ass.
    
这件事引发了 Linux 社区中不同层次的人有不同层次的思考，包括：

- 内核维护者（CPU体系结构无关的代码）
- 维护ARM系统结构代码的人 
- 维护ARM sub architecture的人（来自各个ARM SOC vendor）

> 维护ARM sub architecture 的人并没有强烈的使命感，作为公司的一员，他们最大的目标是以最快的速度支持自己公司的 SOC，尽快的占领市场。这些人的软件功力未必强，对 linux kernel 的理解未必深入（有些人可能很强，但是人在江湖身不由己）。在这样的情况下，很多 SOC specific 的代码都是通过 copy and paste，然后稍加修改代码就提交了。此外，各个 ARM vendor 的 SOC family 是一长串的 CPU list，每个 CPU 多多少少有些不同，这时候 ＃ifdef 就充斥了各个源代码中，让 ARM mach- 和 plat- 目录下的代码有些不忍直视。

> 作为维护 ARM 体系结构的人，其能力不容置疑。以 Russell King 为首的 team 很好的维护了 ARM 体系结构的代码。基本上，除了 mach- 和 plat- 目录，其他的目录中的代码和目录组织是很好的。作为 ARM linux 的维护者，维护一个不断有新的 SOC 加入的 CPU architecture code 的确是一个挑战。在 Intel X86 的架构一统天下的时候，任何想正面攻击 Intel 的对手都败下阵来。想要击倒巨人（或者说想要和巨人并存）必须另辟蹊径。ARM 的策略有两个，一个是 focus 在嵌入式应用上，也就意味着要求低功耗，同时也避免了和 Intel 的正面对抗。另外一个就是博采众家之长，采用 license IP 的方式，让更多的厂商加入 ARM 建立的生态系统。毫无疑问，ARM 公司是成功的，但是这种模式也给 ARM linux 的维护者带来了噩梦。越来越多的芯片厂商加入 ARM 阵营，越来越多的 ARM platform 相关的代码被加入到内核，不同厂商的周边 HW block 设计又各不相同……

> 内核维护者是真正对操作系统内核软件有深入理解的人，他们往往能站在更高的层次上去观察问题，发现问题。Linus 注意到每次 release 时，ARM 的代码变化大约占整个 ARCH 目录的60％，他认为这是一个很明显的符号，意味着 ARM linux 的代码可能存在问题。其实，60％这个比率的确很夸张，因为 unicore32 是在 2.6.39 merge window 中第一次全新提交，它的代码是全新的，但是其代码变化大约占整个 ARCH 目录的 9.6％ （需要提及的是 unicore32 是一个中国芯）。有些维护 ARM linux 的人认为这是 CPU 市场占用率的体现，不是问题，直到内核维护者贴出实际的代码并指出问题所在。内核维护者当然想 linux kernel 支持更多的硬件平台，但是他们更愿意为 linux kernel 制定更长远的规划。例如：对于各种繁杂的 ARM 平台，用一个 kernel image 来支持。 

经过争论，确定的问题如下：

- 各个 SOC 之间缺乏协调，导致arm linux的代码有重复。不过在本次争论之前，ARM维护者已经进行了不少相关的工作（例如PM和clock tree）来抽象相同的功能模块。
- ARM linux 中大量的 board specific 的源代码应该踢出 kernel，否则这些代码和 table 会影响 linux kernel 的长期目标。
- 各个sub architecture 的维护者直接提交给 Linux 并入主线的机制缺乏层次。

### 解决之道

ARM Linux 问题的解决从人员和重复代码两方面入手：

- 解决人员的问题，也就是解决如何整合各个 ARM sub architecture 的资源，为此内核社区成立了 ARM sub architecture team，这个 team 主要负责协调各个 ARM 厂商的代码；此外建立 ARM platform consolidation tree，ARM sub architecture team 负责 review 各个 sub architecture 维护者提交的代码，并在 ARM platform consolidation tree 上维护，在下一次 release 时，将 patch 发送给 Linus
- 重复代码的问题，内核社区确定：
    
    - ARM 的核心代码仍然保存在 arch/arm 目录下
    - ARM SoC core architecture code （各个 ARM  sub architecture 维护者编写用于和 framework 交互的代码）保存在 arch/arm 目录下
    - ARM SOC 的周边外设模块的驱动保存在 drivers 目录下
    - ARM SOC 的特定代码在 arch/arm/mach-xxx 目录下
    - board-xxx.c 中，移除驱动 SOC 的代码，改使用 Device Tree 机制按照Dtb文件（Device Tree 用来描述硬件拓扑和硬件资源信息的二进制文件）的描述驱动 SOC
    
## Device Tree

Device Tree 改变了原来用 hardcode 方式将 HW 配置信息嵌入到内核代码的方法，改用 bootloader 传递一个 dtb 地址的形式。对于基于 ARM CPU 的嵌入式系统，针对每一个 platform 进行内核的编译。但是随着 ARM 在消费类电子上的广泛应用（甚至桌面系统、服务器系统），社区期望 ARM 像 X86 那样用一个 kernel image 来支持多个 platform。

因此，社区希望使用 Device Tree 机制将下面硬件信息传递给内核，由内核自动驱动SOC：

- 设备的拓扑结构以及特性
- 识别 platform 的信息
- runtime 的配置参数

### DTS 和 DTB

DTS 文件用于以人类语言描述 Device Tree ，通过DTC（Device Tree Compiler），可以将这些适合人类阅读的Device Tree source file变成适合机器处理的 device tree blob。

在系统启动的时候，bootloader 可以将保存在闪存中的 DTB 加载到内存，并将 DTB 的起始地址传递给内核

### Device Tree 的结构

Device Tree 由一系列被命名的结点（node）和属性（property）组成，而结点本身可包含子结点。所谓属性，其实就是成对出现的 name 和 value。在 Device Tree中，可描述的信息包括：

- CPU的数量和类别等
- 内存基地址和大小等
- 总线、总线控制器、bridge 等
- 外设连接
- 中断控制器和中断使用情况
- GPIO控制器和GPIO使用情况
- Clock控制器和Clock使用情况

#### node

一个 device tree 文件中只能有一个 root node

每个 node 中包含了若干的 property/value 来描述该 node 的一些特性

每个 node 用节点名字（ node name）标识，节点名字的格式是 node-name@unit-address； node 没有 reg 属性，那么该节点名字中必须不能包括 @和 unit-address

unit-address 的具体格式是和设备挂在那个bus上相关，例如对于 cpu，其 unit-address 就是从0开始编址，依次加一。而具体的设备，例如以太网控制器，其 unit-address 就是寄存器地址

在 device tree 中，引用 node 需要指定 node 的全路径，如 /cpus/cpu@0

#### label 的定义和使用

在一个 node 前使用 ***: 可以为这个 node 定义标签，如 msmgpio: gpio@fd510000， msmgpio 是 gpio@fd510000 的 label

为 node 定义标签后，在 device tree 中就可以使用 lable 引用标签，com,cdc-mclk-gpios = <&pm8941_gpios 15 0>

#### 属性

属性值可以为空

属性值是 u32 或 u64 的值或数组时，使用 <> 标记, 如 <1> <57900 88200 99600 138800 149600 170200 178300 189100 232100 256500 266400 287700 325700 386200>

属性值是字符串，使用 "" 标记, 如 "cpu"

一些属性名使用 xxx,yyy 这样的格式，比如 qcom,board-id，qcom,linux,reserve-contiguous-regionmsm-id。xxx 表示属性的作用于，比如 qcom 表示该属性只能用于高通平台，linux 表示属性适用于一切支持 linux 的平台

以 msm8974 的 Device Tree 结构为例：
```
 / o device-tree
      |- name = "/"
      |- model = "Qualcomm MSM 8974"
      |- compatible = "qcom,msm8974-mtp", "qcom,msm8974", "qcom,mtp"
      |- #address-cells = <1>
      |- #size-cells = <1>
      |- qcom,board-id = <8 0>
      |- qcom,msm-id = <194 0x10000>,<210 0x10000>,<213 0x10000>,<216 0x10000>;
      |- interrupt-parent = <&intc>
      |
      o cpus
      | | - name = "cpus"
      | | - #address-cells = <1>
      | | - #size-cells = <0>
      | |
      | o cpu@0
      | | |- name = "cpu@0"
      | | |- compatible = "qcom,krait"
      | | |- device_type = "cpu"
      | | |- reg = <0x0>
      | | |- current = < 57900 88200 99600 138800 149600 170200 178300 189100 232100 256500 266400 287700 325700 386200>
      
      ...
      
      | o cpu@3
      | | |- name = "cpu@3"
      | | |- compatible = "qcom,krait"
      | | |- device_type = "cpu"
      | | |- reg = <0x3>
      | | |- current = < 31266 47628 53784 74952 80784 91908 96282 102114 125334 138510 143856 155358 175878 208548>
      | 
      o memory
      | |- name = "memory"
      | |- device_type = "memory"
      | |
      | o secure_mem: secure_region
      | | |- name = "secure_region"
      | | |- linux,reserve-contiguous-region
      | | |- reg = <0 0xFC00000>
      | | |- label = "secure_mem"
      | |
      | o adsp_mem: adsp_region
      | | |- name = "adsp_region"
      | | |- linux,reserve-contiguous-region
      | | |- reg = <0 0x4100000>
      | | |- label = "adsp_mem"
      |
      o chosen
      | |- name = "chosen"
      | |- bootargs
      |
      o soc
      | |- name = "soc"
      | |- bootargs = "root=/dev/sda2"
      | |- ranges
      | |
      | o interrupt-controller@F9000000
      | | |- name = "interrupt-controller@F9000000"
      | | |- compatible = "qcom,msm-qgic2"
      | | |- #interrupt-cells = <3>
      | | |- reg = <0xF9000000 0x1000>,<0xF9002000 0x1000>
      | | |- label = "intc"
      | |
      
      ...
      
      | o msmgpio: gpio@fd510000
      | | |- name = "msmgpio: gpio@fd510000"
      | | |- compatible = "qcom,msm-gpio"
      | | |- gpio-controller
      | | |- #gpio-cells = <2>
      | | |- interrupt-controller
      | | |- #interrupt-cells = <2>
      | | |- reg = <0xfd510000 0x4000>
      | | |- ngpio = <146>
      | | |- interrupts = <0 208 0>
      | | |- #gpio-cells = <2>
      | | |- qcom,direct-connect-irqs = <8>
      | |
      | o gpios
      | | |- name = "gpios"
      | | |- spmi-dev-container
      | | |- compatible = "qcom,qpnp-pin"
      | | |- gpio-controller
      | | |- #gpio-cells = <2>
      | | |- #address-cells = <1>
      | | |- #size-cells = <1>
      | | |- label = "pm8941-gpio"
      | | |
      | | o gpio@c000
      | | |- name = "gpio@c000"
      | | |- reg = <0xc000 0x100>
      | | |- qcom,pin-num = <1>
      | | o gpio@c100
      | | |- name = "gpio@c100"
      | | |- reg = <0xc100 0x100>
      | | |- qcom,pin-num = <2>
      
      ...
      
      | | o gpio@e300
      | | |- name = "gpio@e300"
      | | |- reg = <0xe300 0x100>
      | | |- qcom,pin-num = <36>
      | |
      | o sound
      | | |- name = "sound"
      | | |- reg = <0xfe02b000 0x4>,<0xfe02c000 0x4>,<0xfe02d000 0x4>,<0xfe02e000 0x4>;
      | | |- compatible = "qcom,msm8974-audio-taiko"
      | | |- qcom,model = "msm8974-taiko-snd-card"
      | | |- reg-names = "lpaif_pri_mode_muxsel","lpaif_sec_mode_muxsel","lpaif_tert_mode_muxsel","lpaif_quat_mode_muxsel"
      | | |- qcom,cdc-mclk-gpios = <&pm8941_gpios 15 0>
      | | |- qcom,taiko-mclk-clk-freq = <9600000>
      | | |- qcom,prim-auxpcm-gpio-clk  = <&msmgpio 65 0>
      | | |- qcom,prim-auxpcm-gpio-sync = <&msmgpio 66 0>
      | | |- qcom,prim-auxpcm-gpio-din  = <&msmgpio 67 0>
      | | |- qcom,prim-auxpcm-gpio-dout = <&msmgpio 68 0>
      | | |- qcom,prim-auxpcm-gpio-set = "prim-gpio-prim"
      | | |- qcom,sec-auxpcm-gpio-clk  = <&msmgpio 79 0>
      | | |- qcom,sec-auxpcm-gpio-sync = <&msmgpio 80 0>
      | | |- qcom,sec-auxpcm-gpio-din  = <&msmgpio 81 0>
      | | |- qcom,sec-auxpcm-gpio-dout = <&msmgpio 82 0>
      | |
      
      ...
      
```

## dts 文件

在linux kernel中，扩展名是 dts 或 dtsi 的文件就是描述硬件信息的 device tree source file， 这些文件放在 kernel/arch/arm/boot/dts/ 下， dts 是 device tree 的入口文件，dtsi 用于可以被 dts 或其他 dtsi include

内核构建时使用 dtc 命令将 dts 编译成 dtb 文件，并放在 boot image 的中，开机时，bootloader 会根据硬件信息将合适的 dtb 加载进内存，然后将 dtb 所在内存的地址等相关信息传入内核，在内核中，根据 dtb 描述的 device tree 信息驱动设备

在 boot image 中， 可以包含多个 dtb ，也可以只包含一个 dtb，比如 qcom 平台，内核构建时，将多个 dtb 编入 dt.img，进入整合进 boot.img

当 boot image 中存在多个 dtb 时，bootloader 通过检查 device tree 根节点下的 compatible、 qcom,board-id、 qcom,msm-id 属性是否与硬件匹配来选择 dtb

### 语法

在 dts 文件中， node 被定义为：
```
[label:] node-name[@unit-address] {
   [properties definitions]
   [child nodes]
} 
```

每个 dts 都需要 include skeleton.dtsi，该文件定义了 device tree 根节点中的基本信息

`skeleton.dtsi`
```
/ {
    #address-cells = <1>;
    #size-cells = <1>;
    chosen { };
    aliases { };
    memory { device_type = "memory"; reg = <0 0>; };
}; 
```
- chosen 节点传递 boot 命令给内核，boot 命令可控制内核引导从 usb 等设备启动。原先 boot 命令是由bootloader 通过 tag last 传递给内核，这里不多做展开
- aliases 节点用于为一些被多次引用的节点定义 alias，这样可以简化一些代码
- memory 节点时必须节点，用于定义物理内存的布局
- device_type 属性定义节点的设备类型，比如 cpu memory serial等
- reg 定义及地点所代表的设备的寄存器的地址和长度。对于设备节点，reg 描述的是 memory-mapped IO register 的 offset 和 length；对于 memory 节点，reg 描述的是这块内存的起始地址和长度
- \#address-cells \#size-cells 属性定义所有子/孙节点的reg属性的默认寄存器地址和长度


以 msm8974.dtsi 为例，高通平台 dts 的基本结构如下：

```
/include/ "skeleton.dtsi"

/ {
	model = "Qualcomm MSM 8974";
	compatible = "qcom,msm8974";
	interrupt-parent = <&intc>;

	cpus {
		#size-cells = <0>;
		#address-cells = <1>;

		CPU0: cpu@0 {

            ...
            
		};

	    ...

		CPU3: cpu@3 {

            ...
            
		};
	};

	memory {

        ...

	};

	soc: soc { };
};

&soc {
    ...
	msmgpio: gpio@fd510000 {
		compatible = "qcom,msm-gpio";
		gpio-controller;
		#gpio-cells = <2>;
		interrupt-controller;
		#interrupt-cells = <2>;
		reg = <0xfd510000 0x4000>;
		ngpio = <146>;
		interrupts = <0 208 0>;
		qcom,direct-connect-irqs = <8>;
	};
    ...
}

```
- model 属性描述 SOC 的生产商和型号
- compatible 属性是一个 string list，定义了一系列 machine 或 device 型号。如果 compatible 属性出现在根节点， 则被用来识别 machine 型号（高通平台通常和 "qcom,board-id"、"qcom,msm-id"一起使用）；如果出现在其他节点，则被用来匹配 driver
- interrupt-parent 属性描述各个HW block的interrupt source是如何物理的连接到interrupt controller。如果 interrupt-parent 属性位于 root node，则是定义了全局的 interrupt-parent 属性，子节点如果没有定义 interrupt-parent 属性，则使用这个全局属性
- interrupts 属性：能产生中断的设备，必须定义interrupts属性，至于属性值，每一个 interrupt controller 的定义都不一样，需要参考文档

如果 dts 或 dtsi 文件引用了其他 dtsi 文件 时，都定义了某个 node， DTC 编译时会将这两个 node 合并

比如 xxx.dts
```
/include/ "msm8974.dtsi"

msmgpio: gpio@fd510000 {
    compatible = "qcom,msm-gpio-1";
};
```

合并后的结果如下：
```
msmgpio: gpio@fd510000 {
    compatible = "qcom,msm-gpio-1";
    gpio-controller;
    #gpio-cells = <2>;
    interrupt-controller;
    #interrupt-cells = <2>;
    reg = <0xfd510000 0x4000>;
    ngpio = <146>;
    interrupts = <0 208 0>;
    qcom,direct-connect-irqs = <8>;
};
```

## 传入运行时参数

linux/arch/arm/kernel/head.S文件定义了 bootloader 和 kernel 的参数传递要求：
```
MMU = off, D-cache = off, I-cache = dont care, r0 = 0, r1 = machine nr, r2 = atags or dtb pointer. 
```
- arm 同时支持 device tree 和 旧的 tag list
- 如果使用 device tree， r1 传入 machine type nr， r2 传入 dtb 地址， 分别保存在 __machine_arch_type 和 __atags_pointer 这两个全局变量中


## Machine 描述符

每一个 board-xxx.c 使用 DT_MACHINE_START 和 MACHINE_END 宏来定义一个 machine 描述符。由于可能存在很多 board-xxx.c 文件，所以在编译的时候，会将这些 machine 描述符同意放在 .arch.info.init 段中作为 machine 描述符列表

在内核中，machine 描述符使用 machine_desc 结构体（定义于 arch/arm/include/asm/mach/arch.h）表示
```c
struct machine_desc {
    unsigned int        nr;        /* architecture number    */
    const char *const     *dt_compat;    /* array of device tree 'compatible' strings    */
	void			(*reserve)(void);/* reserve mem blocks	*/	
    void			(*map_io)(void);/* IO mapping function	*/
	void			(*init_irq)(void);
	void			(*init_machine)(void);
    void			(*init_time)(void);
	void			(*handle_irq)(struct pt_regs *);
    void			(*restart)(enum reboot_mode, const char *);
    struct smp_operations	*smp;				/* SMP operations	*/
};
```
- nr 是 machine type nr ，不过使用 dt 时，不需要通过 nr 来指定 machine 描述符，而是通过 dt_compat 字符串数组来匹配
- 函数指针用于指定驱动程序的初始化函数

## platform 识别

linux/arch/arm/kernel/setup.c 中的 setup_arch 函数是 arm 的入口函数
```c
void __init setup_arch(char **cmdline_p)
{
    const struct machine_desc *mdesc;

...
    /* 根据 dt 来寻找合适的 machine 描述符 */
    mdesc = setup_machine_fdt(__atags_pointer);
    if (!mdesc)
        /* 如果根据 dt 找不到 machine 描述符，则使用旧的 machine nr 和 tag list 设置 machine 描述符*/
        mdesc = setup_machine_tags(__atags_pointer, __machine_arch_type);
    machine_desc = mdesc;
    machine_name = mdesc->name;

    ...
}
```

setup_machine_fdt 函数的实现位于 linux/arch/arm/kernel/devicetree.c 
```c
const struct machine_desc * __init setup_machine_fdt(unsigned int dt_phys)
{
    const struct machine_desc *mdesc, *mdesc_best = NULL;
    
    /* early_init_dt_scan 函数对 dtb 进行有效性检查，同时获取运行时参数、设置 meminfo 等信息 */
    if (!dt_phys || !early_init_dt_scan(
            phys_to_virt(dt_phys)   /* dt_phys 是物理地址，无法直接访问，需要转为虚拟地址 */
            ))
        return NULL;
    
    /* of_flat_dt_match_machine 函数通过 arch_get_next_mach 函数扫描machine描述符列表，找出最合适的文件描述符 */
    mdesc = of_flat_dt_match_machine(mdesc_best, arch_get_next_mach);

    if (!mdesc) { 
        /* 出错处理 */
        ...
    }

    /* Change machine number to match the mdesc we're using */
    __machine_arch_type = mdesc->nr;

    return mdesc;
} 
```

of_flat_dt_match_machine 函数的实现位于 drivers/of/fdt.c， 用于寻找最匹配的 machine 描述符

```c
const void * __init of_flat_dt_match_machine(const void *default_match,
		const void * (*get_next_compat)(const char * const**))
{
	const void *data = NULL;
	const void *best_data = default_match;
	const char *const *compat;
	unsigned long dt_root;
	unsigned int best_score = ~1, score = 0;    /* best_score 初始化为 unsigned int 最大值减一 */
    
    /* 获取 dt 的 root node */
	dt_root = of_get_flat_dt_root();
    /* 遍历所有 machine 描述符，使用其中的 dt_compat 匹配 dt 中所有的 compatible 属性*/
	while ((data = get_next_compat(&compat))) {
		score = of_flat_dt_match(dt_root, compat);
		if (score > 0 && score < best_score) {
			best_data = data;       /* best_score 得分最低时，取到的是最匹配的 machine 描述符 */
			best_score = score;     /* best_score 取得分最低者 */
		}
	}
	if (!best_data) {
        /* 出错处理 */
        ...
	}

	return best_data;
}
```

arch_get_next_mach 函数在循环中获取下一个 machine 描述符
```c
static const void * __init arch_get_next_mach(const char *const **match)
{
    /* mdesc 是静态常量，只会被初始化一次 */
	static const struct machine_desc *mdesc = __arch_info_begin;    /* __arch_info_begin指向machine描述符列表第一个entry */
	const struct machine_desc *m = mdesc;   /* 通过 m 来访问 machine 描述符 */

	if (m >= __arch_info_end)
		return NULL;

	mdesc++;    /* 遍历 machine 描述符时，需要预先将 mdesc 指向下一个 machine 描述符 */
	*match = m->dt_compat;
	return m;
}
```

of_flat_dt_match 函数将指定 machine 描述符中的 dt_compat 和 dt 中的 compatible 属性进行匹配，并返回最匹配得分
```c
int __init of_flat_dt_match(unsigned long node, const char *const *compat)
{
	return of_fdt_match(initial_boot_params, node, compat);
}
```

```c
int of_fdt_match(struct boot_param_header *blob, unsigned long node,
                 const char *const *compat)
{
	unsigned int tmp, score = 0;

	if (!compat)
		return 0;
    
    /* 将所有 compat 都进行匹配，并返回最匹配的得分 */
	while (*compat) {
		tmp = of_fdt_is_compatible(blob, node, *compat);
		if (tmp && (score == 0 || (tmp < score)))
			score = tmp;    /* 得分越低越匹配 */
		compat++;
	}

	return score;
}
```

```c
int of_fdt_is_compatible(struct boot_param_header *blob,
		      unsigned long node, const char *compat)
{
	const char *cp;
	unsigned long cplen, l, score = 0;

	cp = of_fdt_get_property(blob, node, "compatible", &cplen);
	if (cp == NULL)
		return 0;
    /* 将 dt root 节点中所有 compatible 属性与传入的 compat 比较，返回得分*/
	while (cplen > 0) {
		score++;    /* 比较次数越多，得分越高 */
		if (of_compat_cmp(cp, compat, strlen(compat)) == 0)
			return score;
		l = strlen(cp) + 1;
		cp += l;
		cplen -= l;
	}

	return 0;
}
```

## 获取运行时参数 
在 setup_machine_fdt 函数 中，使用 early_init_dt_scan 函数（实现位于 devices/of/fdt.c ）对 dtb 进行有效性检查，同时获取运行时参数、设置 meminfo 等信息
```c
bool __init early_init_dt_scan(void *params)
{
    if (!params)
        return false;

    /* 全局变量 initial_boot_params 指向了 DTB 的 header*/
    initial_boot_params = params;

    /* 检查 DTB 的 magic ，确认是一个有效的 DTB */
    if (be32_to_cpu(initial_boot_params->magic) != OF_DT_HEADER) {
        initial_boot_params = NULL;
        return false;
    }

    /* 扫描 /chosen node，保存运行时参数（bootargs）到 boot_command_line，此外，还处理 initrd 相关的 property ，并保存在全局变量 initrd_start 和 initrd_end 中 */
    of_scan_flat_dt(early_init_dt_scan_chosen, boot_command_line);

    /* 扫描根节点，获取 {size,address}-cells 信息，并保存在 dt_root_size_cells 和 dt_root_addr_cells 全局变量中 */
    of_scan_flat_dt(early_init_dt_scan_root, NULL);

    /* 扫描 DTB 中的 memory node，并把相关信息保存在 meminfo 中，全局变量 meminfo 保存了系统内存相关的信息。*/
    of_scan_flat_dt(early_init_dt_scan_memory, NULL);

    return true;
} 
```

## 将 DT 转换为 device_node tree

device_node 结构体被用来抽象 device tree 中的一个节点
```c
struct device_node {
    const char *name;               /* device node name */
    const char *type;               /* 对应device_type的属性 */
    phandle phandle;                /* 对应该节点的phandle属性 */
    const char *full_name;          /* 从“/”开始的，表示该node的full path */

    struct property *properties;    /* 该节点的属性列表 */
    struct property *deadprops;     /* 如果需要删除某些属性，kernel并非真的删除，而是挂入到deadprops的列表 */
    struct device_node *parent;     /* parent、child以及sibling将所有的device node连接起来 */
    struct device_node *child;
    struct device_node *sibling;
    struct device_node *next;       /* －通过该指针可以获取相同类型的下一个node */
    struct device_node *allnext;    /* 通过该指针可以获取node global list下一个node */
    struct proc_dir_entry *pde;     /* 开放到userspace的proc接口信息 */
    struct kref kref;               /* 该node的reference count */
    unsigned long _flags;
    void    *data;
};

```

setup_arch 函数中调用 unflatten_device_tree 扫描 DT，并将其组织为：
- 全局的 device_node 列表， of_allnodes 指向这个全局列表，用于快速访问所有的 device tree 节点
- device_node tree
```c
void __init unflatten_device_tree(void)
{
	__unflatten_device_tree(initial_boot_params, &of_allnodes,
				early_init_dt_alloc_memory_arch);

	/* Get pointer to "/chosen" and "/aliases" nodes for use everywhere */
	of_alias_scan(early_init_dt_alloc_memory_arch);
}
```

```c
static void __unflatten_device_tree(struct boot_param_header *blob,
			     struct device_node **mynodes,
			     void * (*dt_alloc)(u64 size, u64 align))
{
	unsigned long size;
	void *start, *mem;
	struct device_node **allnextp = mynodes;
    
    /* dtb 检查 */
	...

	/* scan过程分成两轮，第一轮主要是确定存储所有node和property所需的内存size */ 
	start = ((void *)blob) + be32_to_cpu(blob->off_dt_struct);
	size = (unsigned long)unflatten_dt_node(blob, 0, &start, NULL, NULL, 0);    /* unflatten_dt_node 函数被用来 scan dt */
	size = ALIGN(size, 4);

	pr_debug("  size is %lx, allocating...\n", size);

	/* 一次性为所有 node 和 property 分配所有内存 */
	mem = dt_alloc(size + 4, __alignof__(struct device_node));
	memset(mem, 0, size);

	*(__be32 *)(mem + size) = cpu_to_be32(0xdeadbeef);  /* 用来检验后面unflattening是否溢出 */

	pr_debug("  unflattening %p...\n", mem);

	/* 第二轮的scan，构建device node tree */ 
	start = ((void *)blob) + be32_to_cpu(blob->off_dt_struct);
	unflatten_dt_node(blob, mem, &start, NULL, &allnextp, 0); /* of_allnodes 复制给 allnextp 传入 unflatten_dt_node，构建全局 device_node 列表 */
    
    /* 校验溢出和校验OF_DT_END */
	if (be32_to_cpup(start) != OF_DT_END)
		pr_warning("Weird tag at end of tree: %08x\n", be32_to_cpup(start));
	if (be32_to_cpup(mem + size) != 0xdeadbeef)
		pr_warning("End of tree marker overwritten: %08x\n",
			   be32_to_cpup(mem + size));
	*allnextp = NULL;

	pr_debug(" <- unflatten_device_tree()\n");
}
```

## 将 dt 并入 Linux 设备驱动模型

将 device 挂入 bus 链表由两种方式：

- 对于即插即用类型的总线 ，在插入一个设备后，总线可以检测到这个行为并动态分配一个 device 结构（一般是 xxx_device ，例如 usb_device ），之后，将该数据结构挂入 bus 上的 device 链表。然后通过 bus 的 match 函数来匹配 driver 和 device。
- 对于非即插即用型总线，如 platform 总线，则需要定义一个 static struct platform_device *xxx_devices 静态数组，在初始化的时候调用 platform_add_devices 函数。这些静态定义的 platform_device 往往又需要静态定义各种 resource，这导致静态表格进一步增大。如果 ARM linux 中不再定义这些表格，那么一定需要一个转换的过程，也就是说，引入 Device Tree 机制后，系统根据 DT 的描述在系统中添加 platform_device。

当然，不是所有的 device node 都会挂入 bus 的设备链表，如 cpus
 memory choose 节点等
 
### cpus 节点的处理

在 setup_arch 函数中调用 arm_dt_init_cpu_maps 函数处理 cpus 节点，详情可参考 linux/Documentation/devicetree/bindings/arm/cpus.txt
```c
void __init arm_dt_init_cpu_maps(void)
{
    ... 
    /* 扫描 device node global list，寻找 full path 是“/cpus”的那个device node。cpus这个device node只是一个容器，其中包括了各个cpu node的定义以及所有cpu node共享的property。*/
	cpus = of_find_node_by_path("/cpus");

	if (!cpus)
		return;

	for_each_child_of_node(cpus, cpu) { /*  遍历cpus的所有的child node */
		u32 hwid;

		if (of_node_cmp(cpu->type, "cpu"))  /* 我们只关心那些device_type是cpu的node */
			continue;

		/* 读取reg属性的值并赋值给hwid */
		if (of_property_read_u32(cpu, "reg", &hwid)) {
			return;
		}

		/* reg的属性值的高8位必须设置为0，这是ARM CPU binding定义的。 */
		if (hwid & ~MPIDR_HWID_BITMASK)
			return;

		/* 不允许重复的CPU id，那是一个灾难性的设定 */
		for (j = 0; j < cpuidx; j++)
			if (WARN(tmp_map[j] == hwid, "Duplicate /cpu reg "
						     "properties in the DT\n"))
				return;

		/* 数组 tmp_map 保存了系统中所有 CPU 的 MPIDR 值（CPU ID值），具体的 index 的编码规则是： tmp_map[0] 保存了 booting CPU 的 id 值，其余的 CPU 的 ID 值保存在 1～NR_CPUS 的位置。 */
		if (hwid == mpidr) {    /* reg 等于 mpidr，该 cpu 为 booting cpu */
			i = 0;
			bootcpu_valid = true;
		} else {
			i = cpuidx++;
		}

		if (WARN(cpuidx > nr_cpu_ids, "DT /cpu %u nodes greater than "
					       "max cores %u, capping them\n",
					       cpuidx, nr_cpu_ids)) {
			cpuidx = nr_cpu_ids;
			break;
		}

		tmp_map[i] = hwid;
	}

	/* 根据DTB中的信息设定 cpu logical map 数组。 */
	for (i = 0; i < cpuidx; i++) {
		set_cpu_possible(i, true);
		cpu_logical_map(i) = tmp_map[i];
		pr_debug("cpu logical map 0x%x\n", cpu_logical_map(i));
	}
}
```

### memory 节点的处理

在 setup_arch 函数中调用 setup_arch->setup_machine_fdt->early_init_dt_scan->early_init_dt_scan_memory 函数处理 cpus 节点
```c
int __init early_init_dt_scan_memory(unsigned long node, const char *uname,
				     int depth, void *data)
{
	char *type = of_get_flat_dt_prop(node, "device_type", NULL);
	__be32 *reg, *endp;
	unsigned long l;

	/* 在初始化的时候，我们会对每一个 device node 都要调用该 call back 函数，因此，我们要过滤掉那些和 memory block 定义无关的 node。和 memory block 定义有的节点有两种，一种是 node name 是 memory@ 形态的，另外一种是 node 中定义了 device_type 属性并且其值是 memory。 */
	if (type == NULL) {
		if (depth != 1 || strcmp(uname, "memory@0") != 0)
			return 0;
	} else if (strcmp(type, "memory") != 0)
		return 0;

    /* 通过读取 reg 或 "linux,usable-memory" 属性获取 memeory 起始地址和长度的信息 */
	reg = of_get_flat_dt_prop(node, "linux,usable-memory", &l);
	if (reg == NULL)
		reg = of_get_flat_dt_prop(node, "reg", &l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));

	pr_debug("memory scan node %s, reg size %ld, data: %x %x %x %x,\n",
	    uname, l, reg[0], reg[1], reg[2], reg[3]);

    /* reg 属性的值是 address，size 数组，那么如何来取出一个个的 address/size 呢？由于 memory node 一定是 root node 的 child，因此 dt_root_addr_cells （root node 的#address-cells属性值）和 dt_root_size_cells （root node的#size-cells属性值） 之和就是 address，size 数组的 entry size。 */ 
	while ((endp - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
		u64 base, size;

		base = dt_mem_next_cell(dt_root_addr_cells, &reg);
		size = dt_mem_next_cell(dt_root_size_cells, &reg);

		if (size == 0)
			continue;
		pr_debug(" - %llx ,  %llx\n", (unsigned long long)base,
		    (unsigned long long)size);

		early_init_dt_add_memory_arch(base, size);
	}

	return 0;
}
```

### machine 初始化

machine 初始化函数由 machine_desc 结构体的 init_machine 函数指针指定
```c
DT_MACHINE_START(MSM8974_DT, "Qualcomm MSM 8974 (Flattened Device Tree)")
    ...
	.init_machine = msm8974_init
    ...
MACHINE_END
```

init_machine 函数被 arch/arm/kernel/setup.c 中的  customize_machine 函数调用
```c
static int __init customize_machine(void)
{
	/*
	 * customizes platform devices, or adds new ones
	 * On DT based machines, we fall back to populating the
	 * machine from the device tree, if no callback is provided,
	 * otherwise we would always need an init_machine callback.
	 */
	if (machine_desc->init_machine)
		machine_desc->init_machine();
#ifdef CONFIG_OF
	else
		of_platform_populate(NULL, of_default_bus_match_table,
					NULL, NULL);
#endif
	return 0;
}
arch_initcall(customize_machine);
```

arch_initcall 宏会将 customize_machine 函数链接到 .initcall3.init 段中，内核启动的时候，通过 starup_kernel->rest_init->kernel_init->kernel_init_freeable->do_base_setup_do_initcalls 执行

init_machine 指针指定 msm8974_init 函数为 msm8974 平台的 machine init 函数，其内部通过调用 of_platform_populate 函数来生成 platform device

of_platform_populate 首先从 of_allnodes 中找到 root 节点或指定的节点，然后扫描其下的所有子节点（不扫描孙节点），并调用 of_platform_bus_create 生成 platform device
```c
int of_platform_populate(struct device_node *root,
			const struct of_device_id *matches,
			const struct of_dev_auxdata *lookup,
			struct device *parent)
{
	struct device_node *child;
	int rc = 0;

	root = root ? of_node_get(root) : of_find_node_by_path("/");
	if (!root)
		return -EINVAL;

	for_each_child_of_node(root, child) {
		rc = of_platform_bus_create(child, matches, lookup, parent, true);
		if (rc)
			break;
	}

	of_node_put(root);
	return rc;
}
```
```c
static int of_platform_bus_create(struct device_node *bus,
				  const struct of_device_id *matches,
				  const struct of_dev_auxdata *lookup,
				  struct device *parent, bool strict)
{
	const struct of_dev_auxdata *auxdata;
	struct device_node *child;
	struct platform_device *dev;
	const char *bus_id = NULL;
	void *platform_data = NULL;
	int rc = 0;

	/* 确保device node有compatible属性的代码 */
	if (strict && (!of_get_property(bus, "compatible", NULL))) {
		pr_debug("%s() - skipping %s, no compatible prop\n",
			 __func__, bus->full_name);
		return 0;
	}

    /* 在传入的 lookup table 寻找和该 device node 匹配的附加数据 */
	auxdata = of_dev_lookup(lookup, bus);
	if (auxdata) {
		bus_id = auxdata->name; /* 如果找到，那么就用附加数据中的静态定义的内容 */
		platform_data = auxdata->platform_data;
	}

    /* ARM 提供了 CPU core，除此之外，它设计了 AMBA 总线来连接 SOC 内的各个 block 。符合这个总线标准的 SOC 上的外设叫做 ARM Primecell Peripherals 。如果一个 device node 的 compatible 属性值是 arm,primecell 的话，可以调用 of_amba_device_create 来向 amba 总线上增加一个 amba device */
	if (of_device_is_compatible(bus, "arm,primecell")) {
		/*
		 * Don't return an error here to keep compatibility with older
		 * device tree files.
		 */
		of_amba_device_create(bus, bus_id, platform_data, parent);
		return 0;
	}

    /* 如果不是 ARM Primecell Peripherals，那么我们就需要向 platform bus 上增加一个 platform device */
	dev = of_platform_device_create_pdata(bus, bus_id, platform_data, parent);
	if (!dev || !of_match_node(matches, bus))
		return 0;

    /* 一个 device node 可能是一个桥设备，因此要重复调用 of_platform_bus_create 来处理所有的 device node */
	for_each_child_of_node(bus, child) {
		pr_debug("   create child: %s\n", child->full_name);
		rc = of_platform_bus_create(child, matches, lookup, &dev->dev, strict);
		if (rc) {
			of_node_put(child);
			break;
		}
	}
	return rc;
}

```
具体增加 platform device 的代码在 of_platform_device_create_pdata 中
```c
static struct platform_device *of_platform_device_create_pdata(
					struct device_node *np,
					const char *bus_id,
					void *platform_data,
					struct device *parent)
{
	struct platform_device *dev;

	if (!of_device_is_available(np))    /* status属性，确保是enable */
		return NULL;

    /* of_device_alloc 除了分配 struct platform_device 的内存，还分配了该 platform device 需要的 resource 的内存（参考 struct platform_device 中的 resource 成员）。当然，这就需要解析该 device node 的 interrupt 资源以及 memory address 资源 */
	dev = of_device_alloc(np, bus_id, parent);
	if (!dev)
		return NULL;

#if defined(CONFIG_MICROBLAZE)
	dev->archdata.dma_mask = 0xffffffffUL;
#endif
    /* 设定platform_device 中的其他成员  */
	dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	if (!dev->dev.dma_mask)
		dev->dev.dma_mask = &dev->dev.coherent_dma_mask;
	dev->dev.bus = &platform_bus_type;
	dev->dev.platform_data = platform_data;

	/* We do not fill the DMA ops for platform devices by default.
	 * This is currently the responsibility of the platform code
	 * to do such, possibly using a device notifier
	 */

	if (of_device_add(dev) != 0) {  /* 把这个 platform device 加入统一设备模型系统中 */
		platform_device_put(dev);
		return NULL;
	}

	return dev;
}
```







