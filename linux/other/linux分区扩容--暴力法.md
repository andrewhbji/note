# Linux 分区扩容 -- 暴力法
##引言
之前一直使用VMware虚拟机运行AOSP构建环境. 随着android 源码越来越庞大,导致原先设定的虚拟硬盘空间不足,因而有了对Linux分区扩容的需求. 一般常用两种方法扩容-- 暴力法和lvm法
### 方法一 : 暴力法
#### 原理
暴力法即合并相邻分区, 使用暴力法扩容前需要注意:
1. 分区使用ext2或更高版本的ext文件系统
2. 分区所在的硬盘仍有剩余空间
3. 剩余空间必须紧跟在待扩容的分区之后
4. 只适合于单个硬盘的情况, 多个跨硬盘合并是不行的
5. 该操作有危险, 务必注意备份数据
#### 步骤
#####1. 检查环境
以我的环境为例: Ubuntu Server 14.04.3 LTS
原虚拟磁盘空间是200G, 后扩容为300G
使用df -Th 查看 主分区/dev/sda1 为195G, 使用ext4文件系统
```shell
#df -Th
文件系统       类型      容量  已用  可用 已用% 挂载点
/dev/sda1      ext4      195G  103G  177G   53% /
none           tmpfs     4.0K     0  4.0K    0% /sys/fs/cgroup
udev           devtmpfs  3.9G   12K  3.9G    1% /dev
tmpfs          tmpfs     797M  3.1M  794M    1% /run
none           tmpfs     5.0M     0  5.0M    0% /run/lock
none           tmpfs     3.9G     0  3.9G    0% /run/shm
none           tmpfs     100M     0  100M    0% /run/user
```
使用fdisk -l 看到主硬盘/dev/sda 总量为322.1 GB, 包含三个分区设备, sda5位swap, 包含在扩展分区sda2中, sda2和sda1之间只相差2047
```shell
#fdisk -l
Disk /dev/sda: 322.1 GB, 322122547200 bytes
255 heads, 63 sectors/track, 39162 cylinders, total 629145600 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x000de0ff

设备 启动      起点          终点     块数   Id  系统
/dev/sda1   *        2048   427050495   213524224   83  Linux
/dev/sda2       427052542   429145599     1046529    5  扩展
/dev/sda5       427054590   429145599     1045505   82  Linux 交换 / Solaris
```
虚拟磁盘扩容后的约100G剩余空间位于硬盘的最后, 所以扩容的时候, 需要把删掉swap, 然后扩容sda1, 最后重新建立swap. 芮然resize2fs支持在线扩容,但为了安全起见, 还是在recovery模式操作
#####2. 在rescue模式中运行shell

从光驱启动 -> 选择recovery broken system -> 跳过前面的系统设置, 直接进入rescue模式 -> 选择不使用root 文件系统 -> 选择在安装环境中运行shell
#####3. 删除 swap 分区
进入fdisk, 输入d, 选择2删除sda2, 然后输入w保存
```shell
#fdisk /dev/sda
命令(输入 m 获取帮助)： d
分区号 (1-5): 2

命令(输入 m 获取帮助)： p

Disk /dev/sda: 322.1 GB, 322122547200 bytes
255 heads, 63 sectors/track, 39162 cylinders, total 629145600 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x000de0ff

   设备 启动      起点          终点     块数   Id  系统
/dev/sda1   *        2048   627050495   313524224   83  Linux

命令(输入 m 获取帮助)： w
```
#####4. 准备扩容
检查并清理/dev/sda1
```shell
#fsck.ext4 -n /dev/sda1
#fsck.ext4 -f /dev/sda1
#tune2fs -O ^has_journal /dev/sda1
```
#####5. 修改分区表
删除sda1分区,然后在原扇区的基础上重建分区,并增加其大小.
```shell
# fdisk /dev/sda

命令(输入 m 获取帮助)： d
分区号 (1-5): 1

命令(输入 m 获取帮助)： p

Disk /dev/sda: 322.1 GB, 322122547200 bytes
255 heads, 63 sectors/track, 39162 cylinders, total 629145600 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x000de0ff

   设备 启动      起点          终点     块数   Id  系统

命令(输入 m 获取帮助)： n
Partition type:
   p   primary (0 primary, 0 extended, 4 free)
   e   extended
Select (default p):
Using default response p
分区号 (1-4，默认为 1)：
将使用默认值 1
起始 sector (2048-629145599，默认为 2048)：
将使用默认值 2048
Last sector, +扇区 or +size{K,M,G} (2048-629145599，默认为 629145599)： 627050495

```
重建swap分区:
```shell
命令(输入 m 获取帮助)： n
Partition type:
   p   primary (1 primary, 0 extended, 3 free)
   e   extended
Select (default p): e
分区号 (1-4，默认为 2)：
将使用默认值 2
起始 sector (627050496-629145599，默认为 627050496)： 627052542
Last sector, +扇区 or +size{K,M,G} (627052542-629145599，默认为 629145599)：
将使用默认值 629145599

命令(输入 m 获取帮助)： n
Partition type:
   p   primary (1 primary, 1 extended, 2 free)
   l   logical (numbered from 5)
Select (default p): l
Adding logical partition 5
起始 sector (627054590-629145599，默认为 627054590)： 627054590
Last sector, +扇区 or +size{K,M,G} (627054590-629145599，默认为 629145599)：
将使用默认值 629145599
```
修改swap分区格式
```shell
命令(输入 m 获取帮助)： t
分区号 (1-5): 5
Hex code (type L to list codes): 82
Changed system type of partition 5 to 82 (Linux 交换 / Solaris)

命令(输入 m 获取帮助)： p

Disk /dev/sda: 322.1 GB, 322122547200 bytes
255 heads, 63 sectors/track, 39162 cylinders, total 629145600 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x000de0ff

   设备 启动      起点          终点     块数   Id  系统
/dev/sda1            2048   627050495   313524224   83  Linux
/dev/sda2       627052542   629145599     1046529    5  扩展
/dev/sda5       627054590   629145599     1045505   82  Linux 交换 / Solaris
```
激活启动选项, 保存分区表
```shell
命令(输入 m 获取帮助)： a
分区号 (1-5): 1

命令(输入 m 获取帮助)： p

Disk /dev/sda: 322.1 GB, 322122547200 bytes
255 heads, 63 sectors/track, 39162 cylinders, total 629145600 sectors
Units = 扇区 of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x000de0ff

   设备 启动      起点          终点     块数   Id  系统
/dev/sda1   *        2048   627050495   313524224   83  Linux
/dev/sda2       627052542   629145599     1046529    5  扩展
/dev/sda5       627054590   629145599     1045505   82  Linux 交换 / Solaris

命令(输入 m 获取帮助)： w
```
#####6. 扩容分区
使用resize2fs扩容
```shell
#e2fsck -f /dev/sda1
#resize2fs /dev/sda1
```
#####7. 重建索引
```shell
#fsck -n /dev/sda1
#tune2fs -j /dev/sda1
```
#####8. 格式化swap
```shell
#mkswap /dev/sda5
```
####结果
reboot后, 使用df -Th查看分区
使用df -Th 查看 主分区/dev/sda1 为295G, 扩容成功
```shell
#df -Th
文件系统       类型      容量  已用  可用 已用% 挂载点
/dev/sda1      ext4      295G  103G  177G   37% /
none           tmpfs     4.0K     0  4.0K    0% /sys/fs/cgroup
udev           devtmpfs  3.9G   12K  3.9G    1% /dev
tmpfs          tmpfs     797M  3.1M  794M    1% /run
none           tmpfs     5.0M     0  5.0M    0% /run/lock
none           tmpfs     3.9G     0  3.9G    0% /run/shm
none           tmpfs     100M     0  100M    0% /run/user
```