#Linux系统服务daemons
##什么是daemon
在Unix-Like的系统中，实现service的程序通常被称为daemon
##daemon的分类
- stand_alone daemon
  stand alone daemon可以自行启动，而且不必通过其他机制管理，daemon启动后旧一直占用内存和系统资源。这种类型的daemon包括httpd，vsftpd等
- super daemon
  有一些服务通过一个统一的daemon来加载， 这个daemon被称为super daemon。
##Super Daemon
###Super Daemon服务的生命周期
这一类型的服务的生命周期大致为：
  1. 客户端通过super daemon(如xinetd)唤起service
  2. super daemon和service之间建立联系
  3. 客户端通过super daemon和service通信
  4. 客户端完成运行后，通过super daemon关闭service
###Super Daemon服务的类型
- multi-threaded
所以一个服务同时会负责好几个客户端
- single-threaded
所以一个服务同时会负责一个客户端
##daemon的工作状态
- signal-control
这种 daemon 是透过讯号来管理的，只要有任何客户端的需求进来，他就会立即启动去处理！例如打印机的服务 (cupsd)
- interval-control
这种 daemon 则主要是每隔一段时间就主动的去运行某项工作，所以，你要作的是在配置文件指定服务要进行的时间与工作， 该服务在指定的时间才会去完成工作。如atd和crond
##daemon 的命名规则
服务名+d
##daemon的启动方式
###和daemon相关的路径
- /etc/init.d/* ：启动脚本放置处
- /etc/sysconfig/* ：各服务的初始化环境配置文件
- /etc/xinetd.conf, /etc/xinetd.d/* ：super daemon 配置文件
- /etc/* ：各服务各自的配置文件
- /var/lib/* ：各服务产生的数据库
- /var/run/* ：各服务的程序之 PID 记录处
###Stand alone的启动
- 直接在/etc/init.d/下使用脚本启动
- 使用service命令
根据参数再到 /etc/init.d/ 去取得正确的服务来 start 或 stop
```sh
# service [service name] (start|stop|restart|...)
选项与参数：
service name：亦即是需要启动的服务名称，需与 /etc/init.d/ 对应；
start|...   ：亦即是该服务要进行的工作。
--status-all：将系统所有的 stand alone 的服务状态通通列出来
```
###Super daemon的管理和启动
### 查看Super daemon所管理的服务的状态
查看/etc/xinetd.d/下的文件即可
### 通过Super Daemon启动服务
1. 编辑/etc/xinetd.d/下的配置
2. 重启xinetd
```sh
# 1. 先修改配置文件成为启动的模样：
# vim /etc/xinetd.d/rsync
# 请将 disable 那一行改成如下的模样 (原本是 yes 改成 no 就对了)
service rsync
{
        disable = no
....(后面省略)....

# 2. 重新启动 xinetd 这个服务
# /etc/init.d/xinetd restart

# 3. 观察启动的端口
# grep 'rsync' /etc/services  <==先看看端口是哪一号
rsync           873/tcp               # rsync
rsync           873/udp               # rsync
# netstat -tnlp | grep 873
tcp    0 0 0.0.0.0:873      0.0.0.0:*     LISTEN      4925/xinetd
# 注意看！启动的服务并非 rsync 喔！而是 xinetd ，因为他要控管 rsync 嘛！
# 若有疑问，一定要去看看图 1.1.1 才行！
```
