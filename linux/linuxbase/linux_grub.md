#Linux Grub
##boot loader 的两个stage
1. 运行boot loader主程序
  boot loader主程序必须安装在MBR
2. 主程序加载配置档
  boot loader配置档一般位于/boot下
##grub 的配置档 /boot/grub/menu.lst 与菜单类型
###硬盘与分区在grub中的代号
```
(hd<硬盘序号>,<分区序号>)
```
如(hd0,0)代表第1块硬盘的第一个分区，第一个逻辑分区/dev/sda5的代号是(hd0,4)
###/boot/grub/menu.lst 配置档
- default=0: 默认启动项目
- timeout=5 grub 启动时会进行读秒，如果在 5 秒钟内没有按下任何按键，就会使用 default 后面接的那个 title 项目来启动
- splashimage=(hd0,0)/grub/splash.xpm.gz 启动屏幕背景文件
- hiddenmenu 启动时是否要显示菜单
###启动方式配置
####直接指定核心启动
- root 代表核心文件防止的那个分区而不是根目录
- kernel 内核文件名
- initrd initrd文件名
####利用chain loader的方式转交控制权
- root 其他系统的bootloader所在的分区
- rootnoverify 不验证此分区
- makeactive 将root指定的分区设置为启动分区
- hide 隐藏指定的分区
##创建新的initrd文件
```sh
mkinitrd [-v] [--with=模块名称] initrd档名 核心版本
选项与参数：
-v  ：显示 mkinitrd 的运行过程
--with=模块名称：模块名称指的是模块的名字而已，不需要填写档名。举例来说， 目前核心版本的 ext3 文件系统模块为底下的档名： /lib/modules/$(uname -r)/kernel/fs/ext3/ext3.ko 那你应该要写成： --with=ext3 就好了 (省略 .ko) initrd档名：你所要创建的 initrd 档名，尽量取有意义又好记的名字。
核心版本  ：某一个核心的版本，如果是目前的核心则是 $(uname -r)
```
##安装grub
### 安装grub相关文件
```sh
grub-install [--root-directory=DIR] INSTALL_DEVICE
选项与参数：
--root-directory=DIR 那个 DIR 为实际的目录，使用 grub-install 默认会将 grub 所有的文件都复制到 /boot/grub/* ，如果想要复制到其他目录与硬盘去，就得要用这个参数。
INSTALL_DEVICE 安装grub相关文件到指定硬盘分区
```
###grub shell
-   用"root (hdx,x)"选择含有 grub 目录的那个 partition 代号；
-   用"find /boot/grub/stage1"看看能否找到安装资讯文件；
-   用"find /boot/vmlinuz"看看能否找到 kernel file (不一定要成功！)；
-   用"setup (hdx,x)"或"setup (hdx)"将 grub 安装在 boot sector 或 MBR；
-   用"quit"来离开 grub shell
