#Linux 分区扩容 -- Lvm法
##引言
使用暴力法扩容分区有两个缺点, 给存储管理带来不便:
1. 不能将不相邻的空间合并
2. 不能跨硬盘操作

为解决这个问题, 从Linux 2.4 起加入LVM(Logical Volume Manager)机制, 其特性包括:
1.  LVM可以将一个或多个硬盘的分区在逻辑上组合, 作为一个分区来使用, 当硬盘空间不够使用的时候, 可以继续将其他的硬盘分区加入其中,这样可以实现硬盘空间的动态管理.
2.  允许按用户组管理分区,允许管理员用更直观的名称替代物理磁盘名(sda,sdb等)来表示分区

LVM模型如下图所示

##LVM简介
与传统分区方式相比, LVM在物理分区和文件系统之间加了一个逻辑层, 屏蔽文件系统对物理分区的直接操作,提供了一个抽象的分区. 这个逻辑层包括:
- 物理卷Physical Volume(pv): 实际分区设备(如/dev/sda1)在LVM中的映射
	- physical extent(PE): 每一个pv被划分的基本单元, 具有唯一编号的PE可以被LVM寻址, PE的大小可以被配置, 默认为4MB
- 卷组Volume Group(vg): LVM的中间层, 由pv组成, 功能类似非LVM系统中的硬盘
- 逻辑卷Logical Volume(lv): 在vg基础之上划分的逻辑分区, 功能类似于非LVM系统中的分区, 在lv之上可以建立文件系统
	- logical extent(LE): lv也被划分为LE, 被LVM寻址. 在同一个vg中, LE的大小和PE相同, 并且意义对应

##使用LVM创建分区
假设现有硬盘空余空间5GB, 需要创建LVM系统来管理这5GB空间, 过程如下
1. 创建分区, LVM的分区类型为8e
```shell
# fdisk /dev/sdb

命令(输入 m 获取帮助)： n
Partition type:
   p   primary (0 primary, 0 extended, 4 free)
   e   extended
Select (default p): p
分区号 (1-4，默认为 1)：
将使用默认值 1
起始 sector (2048-10485759，默认为 2048)：
将使用默认值 2048
Last sector, +扇区 or +size{K,M,G} (2048-10485759，默认为 10485759)：
将使用默认值 10485759

命令(输入 m 获取帮助)： t
Selected partition 1
Hex code (type L to list codes): 8e
Changed system type of partition 1 to 8e (Linux LVM)

命令(输入 m 获取帮助)： w
The partition table has been altered!

Calling ioctl() to re-read partition table.
Syncing disks.

# fdisk -l

Disk /dev/sdb: 5368 MB, 5368709120 bytes
181 heads, 40 sectors/track, 1448 cylinders, total 10485760 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x596abe69

   设备 启动      起点          终点     块数   Id  系统
/dev/sdb1            2048    10485759     5241856   8e  Linux LVM

```
2. 创建PV
```shell
# pvcreate /dev/sdb1
  Physical volume "/dev/sdb1" successfully created
```
3. 创建VG
```shell
# vgcreate VolGroup0 /dev/sdb1
  Volume group "VolGroup0" successfully created
```
4. 创建并分配4.9G空间给LV
>note:
>不能将PV全部的空间分配给LV, 否则会报错, 原因不明
```shell
# lvcreate -L 4.9G -n sdb1data VolGroup0
  Rounding up size to full physical extent 4.90 GiB
  Logical volume "sdb1data" created
```
5. 格式化并挂在LV
```shell
# mkfs -t ext4 /dev/VolGroup0/sdb1data
mke2fs 1.42.9 (4-Feb-2014)
文件系统标签=
OS type: Linux
块大小=4096 (log=2)
分块大小=4096 (log=2)
Stride=0 blocks, Stripe width=0 blocks
321280 inodes, 1285120 blocks
64256 blocks (5.00%) reserved for the super user
第一个数据块=0
Maximum filesystem blocks=1317011456
40 block groups
32768 blocks per group, 32768 fragments per group
8032 inodes per group
Superblock backups stored on blocks:
        32768, 98304, 163840, 229376, 294912, 819200, 884736

Allocating group tables: 完成
正在写入inode表: 完成
Creating journal (32768 blocks): 完成
Writing superblocks and filesystem accounting information: 完成
# mkdir /test
# mount /dev/VolGroup0/sdb1data /test
# df -Th
文件系统                       类型      容量  已用  可用 已用% 挂载点
/dev/mapper/ubuntu--vg-root    ext4       29G  1.5G   26G    6% /
none                           tmpfs     4.0K     0  4.0K    0% /sys/fs/cgroup
udev                           devtmpfs  478M  4.0K  478M    1% /dev
tmpfs                          tmpfs      98M  988K   97M    1% /run
none                           tmpfs     5.0M     0  5.0M    0% /run/lock
none                           tmpfs     489M     0  489M    0% /run/shm
none                           tmpfs     100M     0  100M    0% /run/user
/dev/sda1                      ext2      236M   38M  186M   17% /boot
/dev/mapper/VolGroup0-sdb1data ext4      4.8G  9.9M  4.5G    1% /test
```
到这里,格式化后的LV就可以像普通分区一样使用了
但是, 如果需要系统启动的时候自动挂载LV, 在/etc/fstab文件中添加
```
/dev/VolGroup0/sdb1data /test   ext4    errors=remount-ro       0       1
```
##使用LVM扩容分区
假设当前根路径挂载在lvm分区/dev/mapper/ubuntu--vg-root上, /dev/mapper/ubuntu--vg-root在/dev/sda上创建, 使用硬盘/dev/sdb上有空闲空间, 则可以使用LVM为/dev/mapper/ubuntu--vg-root扩容
1. 创建分区, 重启使分区表修改生效
```shell
# fdisk /dev/sdb

命令(输入 m 获取帮助)： n
Partition type:
   p   primary (1 primary, 0 extended, 3 free)
   e   extended
Select (default p): p
分区号 (1-4，默认为 2)：
将使用默认值 2
起始 sector (10485760-12582911，默认为 10485760)：
将使用默认值 10485760
Last sector, +扇区 or +size{K,M,G} (10485760-12582911，默认为 12582911)：
将使用默认值 12582911

命令(输入 m 获取帮助)： p

Disk /dev/sdb: 6442 MB, 6442450944 bytes
181 heads, 40 sectors/track, 1737 cylinders, total 12582912 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x596abe69

   设备 启动      起点          终点     块数   Id  系统
/dev/sdb1            2048    10485759     5241856   8e  Linux LVM
/dev/sdb2        10485760    12582911     1048576   83  Linux

命令(输入 m 获取帮助)： t
分区号 (1-4): 2
Hex code (type L to list codes): 8e
Changed system type of partition 2 to 8e (Linux LVM)

命令(输入 m 获取帮助)： p

Disk /dev/sdb: 6442 MB, 6442450944 bytes
181 heads, 40 sectors/track, 1737 cylinders, total 12582912 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x596abe69

   设备 启动      起点          终点     块数   Id  系统
/dev/sdb1            2048    10485759     5241856   8e  Linux LVM
/dev/sdb2        10485760    12582911     1048576   8e  Linux LVM

命令(输入 m 获取帮助)： w
The partition table has been altered!

Calling ioctl() to re-read partition table.

WARNING: Re-reading the partition table failed with error 16: 设备或资源忙.
The kernel still uses the old table. The new table will be used at
the next reboot or after you run partprobe(8) or kpartx(8)
Syncing disks.
#reboot
```
2. 创建pv
```shell
# pvcreate /dev/sdb2
Physical volume "/dev/sdb2" successfully created
```
3. 查看现有lv和vg, 可以推测/dev/mapper/ubuntu--vg-root属于ubuntu-vg
```shell
# lvdisplay
--- Logical volume ---
LV Path                /dev/ubuntu-vg/root
LV Name                root
VG Name                ubuntu-vg
LV UUID                45nhMO-Bg5D-Bo8J-Oe8T-0toH-7Dqr-OWMcgJ
LV Write Access        read/write
LV Creation host, time ubuntu, 2015-11-27 19:42:16 +0800
LV Status              available
# open                 1
LV Size                28.74 GiB
Current LE             7358
Segments               3
Allocation             inherit
Read ahead sectors     auto
- currently set to     256
Block device           252:1
# vgdisplay
--- Volume group ---
VG Name               ubuntu-vg
System ID
Format                lvm2
Metadata Areas        3
Metadata Sequence No  6
VG Access             read/write
VG Status             resizable
MAX LV                0
Cur LV                2
Open LV               2
Max PV                0
Cur PV                3
Act PV                3
VG Size               29.75 GiB
PE Size               4.00 MiB
Total PE              7873
Alloc PE / Size       7613 / 29.74 GiB
Free  PE / Size       260 / 1.02 GiB
VG UUID               sspZ5V-hEnJ-He0A-orps-G4Zu-pLbH-jJ5bj5
```
4. 扩容ubuntu-vg
```shell
# vgextend ubuntu-vg /dev/sdb2
Volume group "ubuntu-vg" successfully extended
```
5. 扩容lv
```shell
# lvextend -L +1G /dev/ubuntu-vg/root
```
6. 重新分配/dev/mapper/ubuntu--vg-root大小, 重启, 可以看到使用df -Th发现/dev/mapper/ubuntu--vg-root增加了1G
```shell
# resize2fs /dev/mapper/ubuntu--vg-root
# reboot
```
总结
1. 学习LVM过程中, 本人曾设想是否可以将现有非LVM系统直接转为LVM系统, 结果发现使用pvcreate新建pv时, 原有数据会丢失, 所以非LVM系统转LVM系统前,需要备份数据, 另外安装系统的时候, 良好的分区习惯也非常重要, 尽量用不同的分区挂载根路径和/home
2. 很多文章都提及在使用LVM扩容分区的时候, 在创建pv前, 需要格式化新建的分区, 本人在 Linux 3.19.0-25上测试时, 发现不格式化新建分区, 也不会影响扩容结果