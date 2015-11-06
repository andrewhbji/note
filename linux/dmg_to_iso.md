
dmg格式是Mac系统下使用的文件,在Ubuntu下不能直接使用.

iso格式则是大多数Linux和Windows下常用的镜像文件格式.

为了方便在Ubuntu或者Windows下安装Mac虚拟机,需要转换个iso格式出来.

## 过程

1. 打开终端，安装dmg2img

```
sudo apt-get install dmg2img
```

2. 将dmg格式转化为img格式.

```
dmg2img /path/to/example.dmg /path/to/example.img
```

3. 挂载img文件.

```
sudo mkdir /media/example

sudo modprobe hfsplus #创建一个hfsplus文件系统模块

sudo mount -t hfsplus -o loop example.img /media/example # 文件系统挂载到hfsplus文件系统模块
```

4. 用Brasero把img转换成iso格式. Brasero中选数据项目(创建一个数据cd/dvd)，选择上面挂在的img文件

5. 翻刻一个iso就出来了

6. 清理：

```
sudo umount /media/example # 从卸载文件系统

sudo modprobe -r hfsplus # 卸载文件系统模块

sudo rmdir /media/example 删除挂载路径
```
## 总结

这里主要涉及到创建/卸载文件系统模块、挂载/卸载文件系统, 以及使用dmg2img转文件格式

参考资料：

1. http://hi.baidu.com/heutsnc/blog/item/777ac861f68ab25eeaf8f850.html

2. http://webcache.googleusercontent.com/search?q=cache:-uQE6LHwnFcJ:wiki.aranym.org/afs/hd_partition_linux+unrecognized+partition+table+typeOld+situation&cd=2&hl=en&ct=clnk&gl=hk&client=ubuntu

3. http://webcache.googleusercontent.com/search?q=cache:83pRiZ6MMKEJ:www.psibo.unibo.it/areait/partitions.html+bad+superblock+on+/dev/loop0&cd=1&hl=en&ct=clnk&gl=hk&client=ubuntu
