[TOC]
# 第二章 UNIX 标准化和实现

## UNIX 标准化

### ISO C
ISO C标准设计了C语言的24个头文件，目前所有的Unix环境都支持这些头文件

表2.1 ISO C标准定义的头文件

| 头文件     | 说明     |
|:--|:--|
| <assert.h>       | 验证程序断言       |
| <complex.h>       | 复数运算支持       |
| <ctype.h>       | 字符分类和映射支持       |
| <errno.h>       | 错误码       |
| <fenv.h>       | 浮点环境       |
| <float.h>       | 浮点支持       |
| <inttypes.h>       | 整型格式转换       |
| <iso646.h>       | 赋值、关系、一元操作符宏      |
| <limits.h>       | 实现常量       |
| <locale.h>       | 本地化支持       |
| <math.h>       | 数学运算库       |
| <setjmp.h>       | 非局部goto       |
| <signal.h>       | 信号支持       |
| <stdarg.h>       | 可变长度参数       |
| <stdbool.h>       | 布尔类型支持       |
| <stddef.h>      | 标准定义       |
| <stdint.h>       | 标准整型       |
| <stdio.h>       | 标准输入输出       |
| <stdlib.h>       | 实用函数库       |
| <string.h>       | 字符串操作       |
| <tgmath.h>       | 通用类型数学宏       |
| <time.h>       | 时间日期支持       |
| <wchar.h>       | 多字节宽字符支持       |
| <wctype.h>       | 宽字符分类和映射支持       |

### IEEE POSIX
IEEE POSIX定义了可移植操作系统接口标准，分为强制要求部分和可选部分，目前主流UNIX环境支持

表2.2 POSIX标准定义的强制要求部分

| 头文件     | 说明     |
|:--|:--|
| <aio.h>       | 异步IO       |
| <cpio.h>       | cpio打包      |
| <dirent.h>       | 目录项       |
| <dlfcn.h>       | 动态链接       |
| <fcntl.h>       | 文件控制       |
| <fnmatch.h>       | 文件名匹配类型       |
| <glob.h>       | 路径名模式匹配类型       |
| <grp.h>       | 组文件      |
| <iconv.h>       | 编码转换工具       |
| <langinfo.h>       | 语言信息常量       |
| <monetary.h>       | 货币类型和方法       |
| <netdb.h>       | 网络数据库操作       |
| <nl_types.h>       | 消息映射       |
| <poll.h>       | poll函数支持       |
| <pthread.h>       | 线程       |
| <pwd.h>      | passwd文件       |
| <regex.h>       | 正则表达式       |
| <sched.h>       | 执行调度       |
| <semaphore.h>       | 信号量       |
| <strings.h>       | 字符串操作       |
| <tar.h>       | tar打包       |
| <termios.h>       | 终端IO       |
| <unistd.h>       | 符号常量       |
| <wordexp.h>       | 字扩展类型       |
| <arpa/inet.h>       | Internet定义       |
| <net/if.h>       | socket本地接口       |
| <netinet/in.h>       | Internet地址族       |
| <netinet/tcp.h>       | tcp协议定义       |
| <sys/mman.h>       | 内存管理声明       |
| <sys/select.h>       | select方法支持       |
| <sys/socket.h>      | socket接口       |
| <sys/stat.h>       | 文件状态       |
| <sys/statvfs.h>       | 文件系统信息       |
| <sys/times.h>       | 进程时间      |
| <sys/types.h>       | 基本系统数据类型       |
| <sys/un.h>       | UNIX域套接字定义       |
| <sys/utsname.h>       | 系统名       |
| <sys/wait.h>       | 进程控制       |


表2.3 POSIX标准定义的XSI可选头文件

| 头文件     | 说明     |
|:--|:--|
| <fmtmsg.h>       | 消息显示结构       |
| <ftw.h>       | 文件树漫游       |
| <libgen.h>       | 模式匹配函数定义       |
| <ndbm.h>       | 数据库操作       |
| <search.h>       | 搜索表      |
| <syslog.h>       | 系统日志       |
| <utmpx.h>       | 用户帐户数据库       |
| <sys/ipc.h>       | IPC       |
| <sys/msg.h>       | 消息队列       |
| <sys/resource.h>       | 资源操作       |
| <sys/sem.h>       | 信号量       |
| <sys/shm.h>       | 共享存储       |
| <sys/time.h>       | 时间类型       |
| <sys/uio.h>       | 矢量I/O操作       |

### Single Unix Specification
Simple Unix Specification是POSIX.1标准的超集，对POSIX.1进行扩展，以及定义了SUS标准(Unix系统认证，apue中未实际涉及，所以不记录)

| 头文件     | 说明     |
|:--|:--|
| <mqueue.h>       | 消息队列       |
| <spawn.h>       | 实时spawn接口       |

### FIPS
FIPS是联邦信息处理标准，实际上并没有什么卵用，只是美国政府采购的标准，后来还被撤回了

## 限制
Unix环境开发时，经常遇到 跨平台编译的问题，如：x86_64和x86字长的不同导致的数据类型长度不一致。针对此问题，Unix提供两种限制方法
- 编译时限制(头文件、宏定义)
在头文件中对有差异的数据进行宏定义，然后include这些头文件，或者在编译的时候向编译器传入预期的值(如gcc -Demacronname=value xxx.c)
- 运行时限制(sysconf、pathconf、fpathconf)
使用sysconf、pathconf、fpathconf函数取值，由系统提供

### ISO C 的限制
ISO C也提供了编译时限制，C语言标准限制定义在limits.h，float.h对浮点型数据定义，stdio.h中定义了可同时打开标准IO的最大数FOPEN_MAX，和定义tmpnam函数可产生唯一文件名的最大数TMP_MAX

### POSIX 的限制,XSI 的限制
这两种限制没有什么区别

### sysconf,pathconf,fpathconf
- pathconf和fpathconf没有什么区别，只是用文件描述符代替文件路径
- 下面列出sysconf和pathconf可能获取的值，具体参考unistd.h

pathconf
```
_PC_LINK_MAX
        The maximum file link count.

_PC_MAX_CANON
        The maximum number of bytes in terminal canonical input line.

_PC_MAX_INPUT
        The minimum maximum number of bytes for which space is available in a terminal input queue.

_PC_NAME_MAX
        The maximum number of bytes in a file name.

_PC_PATH_MAX
        The maximum number of bytes in a pathname.

_PC_PIPE_BUF
        The maximum number of bytes which will be written atomically to a pipe.

_PC_CHOWN_RESTRICTED
        Return 1 if appropriate privileges are required for the chown(2) system call, otherwise 0.

_PC_NO_TRUNC
        Return 1 if file names longer than KERN_NAME_MAX are truncated.

_PC_VDISABLE
        Returns the terminal character disabling value.

_PC_XATTR_SIZE_BITS
        Returns the number of bits used to store maximum extended attribute size in bytes.  For example, if the maximum attribute size
        supported by a file system is 128K, the value returned will be 18.  However a value 18 can mean that the maximum attribute size
        can be anywhere from (256KB - 1) to 128KB.  As a special case, the resource fork can have much larger size, and some file system
        specific extended attributes can have smaller and preset size; for example, Finder Info is always 32 bytes.
```

sysconf
```
_SC_ARG_MAX
        The maximum bytes of argument to execve(2).

_SC_CHILD_MAX
        The maximum number of simultaneous processes per user id.

_SC_CLK_TCK
        The frequency of the statistics clock in ticks per second.

_SC_IOV_MAX
        The maximum number of elements in the I/O vector used by readv(2), writev(2), recvmsg(2), and sendmsg(2).

_SC_NGROUPS_MAX
        The maximum number of supplemental groups.

_SC_NPROCESSORS_CONF
        The number of processors configured.

_SC_NPROCESSORS_ONLN
        The number of processors currently online.

_SC_OPEN_MAX
        The maximum number of open files per user id.

_SC_PAGESIZE
        The size of a system page in bytes.

_SC_STREAM_MAX
        The minimum maximum number of streams that a process may have open at any one time.

_SC_TZNAME_MAX
        The minimum maximum number of types supported for the name of a timezone.

_SC_JOB_CONTROL
        Return 1 if job control is available on this system, otherwise -1.

_SC_SAVED_IDS
        Returns 1 if saved set-group and saved set-user ID is available, otherwise -1.

_SC_VERSION
        The version of IEEE Std 1003.1 (``POSIX.1'') with which the system attempts to comply.

_SC_BC_BASE_MAX
        The maximum ibase/obase values in the bc(1) utility.

_SC_BC_DIM_MAX
        The maximum array size in the bc(1) utility.

_SC_BC_SCALE_MAX
        The maximum scale value in the bc(1) utility.

_SC_BC_STRING_MAX
        The maximum string length in the bc(1) utility.

_SC_COLL_WEIGHTS_MAX
        The maximum number of weights that can be assigned to any entry of the LC_COLLATE order keyword in the locale definition file.

_SC_EXPR_NEST_MAX
        The maximum number of expressions that can be nested within parenthesis by the expr(1) utility.

_SC_LINE_MAX
        The maximum length in bytes of a text-processing utility's input line.

_SC_RE_DUP_MAX
        The maximum number of repeated occurrences of a regular expression permitted when using interval notation.

_SC_2_VERSION
        The version of IEEE Std 1003.2 (``POSIX.2'') with which the system attempts to comply.

_SC_2_C_BIND
        Return 1 if the system's C-language development facilities support the C-Language Bindings Option, otherwise -1.

_SC_2_C_DEV
        Return 1 if the system supports the C-Language Development Utilities Option, otherwise -1.

_SC_2_CHAR_TERM
        Return 1 if the system supports at least one terminal type capable of all operations described in IEEE Std 1003.2 (``POSIX.2''),
        otherwise -1.

_SC_2_FORT_DEV
        Return 1 if the system supports the FORTRAN Development Utilities Option, otherwise -1.

_SC_2_FORT_RUN
        Return 1 if the system supports the FORTRAN Runtime Utilities Option, otherwise -1.

_SC_2_LOCALEDEF
        Return 1 if the system supports the creation of locales, otherwise -1.

_SC_2_SW_DEV
        Return 1 if the system supports the Software Development Utilities Option, otherwise -1.

_SC_2_UPE
        Return 1 if the system supports the User Portability Utilities Option, otherwise -1.
```

## 功能测试宏
POSIX标准定义了_POSIX_C_SOURCE宏定义和_XOPEN_SOURCE宏定义，用于功能测试

## 基本系统数据类型
为保证跨平台开发而使用的数据类型，基本以_t结尾，这里列出部分基本系统数据类型参考

| 类型     | 说明     |
|:--|:--|
| caddr_t       | 核心地址       |
| clock_t       | 时钟滴答计时器       |
| comp_t       | 压缩的时钟滴答       |
| dev_t       | 设备号       |
| fd_set       | 文件描述符集       |
| fpos_t       | 文件位置       |
| gid_t       | 数值组       |
| ino_t       | i节点编号       |
| mode_t       | 文件类型、文件创建模式       |
| nlink_t       | 目录项的链接数       |
| off_t       | 文件大小和偏移量       |
| pid_t       | 进程ID和进程组ID       |
| ptrdiff_t       | 两个指针相减的结果       |
| rlim_t       | 资源限制       |
| sig_atomic_t       | 能原子访问的数据类型       |
| sigset_t       | 信号集       |
| size_t       | 对象大小       |
| ssize_t       | 返回字节计数的函数       |
| time_t       | 日历时间的秒计数器       |
| uid_t       | 数值用户ID       |
| wchar_t       | 能表示所有不同的字符码       |
