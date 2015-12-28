[TOC]
#Linux ACL
##什么是ACL
ACL 是 Access Control List 的缩写,主要的目的是在提供传统的 owner,group,others 的 read,write,execute权限之外的细部权限配置。ACL 可以针对单一使用者，单一文件或目录来进行r,w,x 的权限规范，对于需要特殊权限的使用状况非常有帮助。
##如何启动ACL
- 绝大部分的文件系统都有支持 ACL 的功能，包括 ReiserFS, EXT2/EXT3, JFS, XFS 等等，可以使用dumpe2fs命令查看
```sh
# dumpe2fs -h /dev/sda1 |grep Default mount optons
Default mount options:    user_xattr acl
```
- 如果系统默认不支持ACL，可以在重新挂在的时候指定支持ACL
```sh
# mount -o remount，acl / #手动挂载
# vi /etc/fstab #开机挂载
LABEL=/1   /   ext3    defaults,acl    1 1
```
##配置ACL setfacl
###格式
```sh
# setfacl [-bkRd] [{-m|-x} acl参数] 目标文件名
```
###常用选项
- -m ：配置后续的 acl 参数给文件使用，不可与 -x 合用；
- -x ：删除后续的 acl 参数，不可与 -m 合用；
- -b ：移除所有的 ACL 配置参数；
- -k ：移除默认的 ACL 参数，关于所谓的『默认』参数于后续范例中介绍；
- -R ：递归配置 acl ，亦即包括次目录都会被配置起来；
- -d ：配置『默认 acl 参数』的意思！只对目录有效，在该目录新建的数据会引用此默认值
##查询ACL getfacl
###格式
```sh
# getfacl filename
```
###查询结果
```sh
# getfacl acl_test1
# file: acl_test1   <==说明档名而已！
# owner: root       <==说明此文件的拥有者，亦即 ll 看到的第三使用者字段
# group: root       <==此文件的所属群组，亦即 ll 看到的第四群组字段
user::rwx           <==使用者列表栏是空的，代表文件拥有者的权限
user:vbird1:r-x     <==针对 vbird1 的权限配置为 rx ，与拥有者并不同！
group::r--          <==针对文件群组的权限配置仅有 r
group:mygroup1:r-x  <==针对mygroup1的权限配置仅有 rx
mask::r-x           <==此文件默认的有效权限 (mask)
other::r--          <==其他人拥有的权限啰！
default:user::rwx
default:user:myuser1:r-x
default:group::rwx
default:mask::rwx
default:other::---
```
>备注：
1. user:vbird1:r-x 代表文件针对vbird的权限是r-x，group:mygroup1:r-x代表文件针对mygroup1的权限是r-x
2. 配置default权限可以用来配置整个目录下所有文件的权限，使目录下所有文件的权限都继承改目录的ACL

##acl参数
###格式
```sh
[d:]u||g||m:[用户名]:权限代码
```
>备注：
1. 当用户名为空时，即无使用者列表。这时此权限将设置给文件拥有者
2. u代表修改用户权限，g代表修改用户组权限,m代表修改mask
3. mask用来修饰user、group的有效权限，如mask为r，user:vbird1为r-x，此时user：vbird
的实际有效权限为r
4. d用来配置default权限
