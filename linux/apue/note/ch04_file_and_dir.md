[TOC]
# 文件和目录

## stat 函数
```c
#include <sys/stat.h>
int stat(const char *restrict path, struct stat *restrict buf);
int fstat(int fildes, struct stat *buf);
int lstat(const char *restrict path, struct stat *restrict buf);
int fstatat(int fd, const char *path, struct stat *buf, int flag);
返回值：若成功返回0，失败则返回-1
```
根据path、符号链接、或者fd获取文件信息
- stat()函数获得路径所指向的文件信息，函数不需要文件的读写执行权限，但是路径树中的所有目录都需要搜索权限(search)
- lstat()函数和stat类似，除了当路径指向为符号链接时，lstat返回链接本身信息，而stat返回对应的文件信息
- fstat()获取已经打开的文件描述符的文件信息
- fstatat()系统调用等价于stat和lstat函数，当AT_FDCWD传入fd参数，并且路径参数为相对路径时，将会计算基于当前工作目录的文件，如果路径为绝对路径，则fd参数被忽略，这个函数实际上是前面几个函数的功能集合版本。

### struct stat 结构体
```c
struct stat {
    mode_t st_mode;           /*文件类型与权限*/
    ino_t st_ino;             /*inode号*/
    dev_t st_dev;             /*dev号*/
    nlink_t st_nlink;         /*链接数*/
    uid_t st_uid;             /*所有者ID*/
    gid_t st_gid;             /*所有者组ID*/
    off_t st_size;            /*大小*/
    struct timespec st_atime; /*最后访问时间*/
    struct timespec st_mtime; /*最后修改时间*/
    struct timespec st_ctime; /*文件状态最后改变时间*/
    blksize_t st_blksize;     /*最佳block大小*/
    blkcnt_t st_blocks;       /*已分配block块数*/
}
```
>备注 timespec 结构体
```c
_STRUCT_TIMESPEC
{
        time_t          tv_sec; /*seconds*/
        long            tv_nsec; /*nanoseconds.*/
};
```

## 文件类型
Unix系统中所有东西都是文件，这就是Unix的哲学，一切皆是文件。文件类型有以下几种
- 普通文件
- 目录文件
- 块文件
- 字符文件
- 管道文件
- 套接字
- 符号链接
<sys/stat.h>中定义了一堆宏来帮助确定文件类型，这些宏用于判断st_mode的类型
```c
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)     /* block special */
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)     /* char special */
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)     /* directory */
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)     /* fifo or socket */
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)     /* regular file */
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)     /* symbolic link */
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)    /* socket */
```
[filetype.c](../examples/filedir/filetype.c)
```c
int i;
struct stat buf;
char *ptr;

printf("%s: ", path);
if (lstat(path, &buf) < 0) {
    err_ret("lstat error");
    continue;
}
if (S_ISREG(buf.st_mode))
    ptr = "regular";
else if (S_ISDIR(buf.st_mode))
    ptr = "directory";
else if (S_ISCHR(buf.st_mode))
    ptr = "character special";
else if (S_ISBLK(buf.st_mode))
    ptr = "block special";
else if (S_ISFIFO(buf.st_mode))
    ptr = "fifo";
else if (S_ISLNK(buf.st_mode))
    ptr = "symbolic link";
else if (S_ISSOCK(buf.st_mode))
    ptr = "socket";
else
    ptr = "unknown mode";
printf("%s\n", ptr);
```

## 用户组ID和组ID
用户组ID和组ID分别保存在 stat 结构体的的st_uid和st_gid中
>备注：对于进程来说关联的ID有6个

  | 各个ID的名称     | 代表的含义     |  备注  |
  |:--|:--| :-------------|
  | 实际用户ID和实际组ID       | 我们实际上是谁       | 比如当前是谁打开的文件  |
  | 有效用户ID、有效组ID和附属组ID       | 用于文件访问权限检查       | 比如，若文件具有 s 权限，则当前用户具有和文件拥有这一样的权限  |
  | 保存的设置用户ID和保存的设置组ID       | 由exec函数保存       | 当前用户使用exec fork子进程，一般情况下子进程仍然由当前用户执行 |

## 文件的访问权限
st_mode 成员还包含文件的访问权限。<sys/stat.h> 也宏定义了一些掩码来帮助确认文件的访问权限(通过stat.st_mode进行位运算)
| st_mode宏定义     | 含义     |
|:--|:--|
| S_IRUSR       | 用户读       |
| S_IWUSR       | 用户写       |
| S_IXUSR       | 用户执行       |
| S_IRGRP       | 组读       |
| S_IWGRP       | 组写       |
| S_IXGRP       | 组执行       |
| S_IROTH       | 其他读       |
| S_IWOTH       | 其他写       |
| S_IXOTH       | 其他执行       |

## 新文件和目录的所有权
- 新文件的UID设置为当前进程的有效UID
- 新文件的GID设置为当前进程有效的GID，或者它所在目录的GID

## access 函数
检查文件或路径的访问权限
```c
#include <unistd.h>
int access(const char *path, int mode);
int faccessat(int fd, const char *path, int mode, int flag);
返回值：若成功返回0，出错则返回-1
```

### mode 参数
mode 参数用于指定要检查的权限

unistd.h 对这些mode值进行了宏定义
| mode     | 说明     |
|:--|:--|
| R_OK       | 测试读权限       |
| W_OK       | 测试写权限       |
| X_OK       | 测试执行权限       |
| F_OK       | 测试文件是否存在      |

[access.c](../examples/filedir/access.c)

```c
/*检查文件的读权限*/
if (access(path, R_OK) < 0)
	err_ret("access error for %s", argv[1]);
else
	printf("read access OK\n");
/*以只读方式打开文件*/
if (open(path, O_RDONLY) < 0)
	err_ret("open error for %s", argv[1]);
else
	printf("open for reading OK\n");
```
>备注：如果 faccessat 的 path 指定相对路径，fd 参数指定为 AT_FDCWD 那么，路径名就在当前工作目录下获取

## umask 函数
作用同umask命令，限制文件的权限

```c
#define RWRWRW (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

umask(0); /*初始化umask，确保umask不受父进程干扰*/
if (creat("foo", RWRWRW) < 0) /*创建文件时，其他group和其他用户默认具备读写权限*/
  err_sys("creat error for foo");
umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); umask(0); /*group和其他用户的读写权限*/
if (creat("bar", RWRWRW) < 0) /*创建文件时，其他group和其他用户不具备读写权限*/
  err_sys("creat error for bar");
exit(0);
```
>备注: 使用open创建文件时，第三个参数指定该文件的权限，如果没有指定权限，默认是 0，即文件没有任何权限

## chmod fchmod 和 chmodat
修改文件权限
```c
#include <sys/stat.h>
int chmod(const char *path, mode_t mode);
int fchmod(int fildes, mode_t mode);
int fchmodat(int fd, const char *path, mode_t mode, int flag);
返回值: 若成功返回0，失败返回-1
```
>备注：如果 fchmodat 的 path 指定相对路径，fd 参数指定为 AT_FDCWD 那么，路径名就在当前工作目录下获取

### mode 常量
chmod 可使用的 mode 常量在 sys/stat.h 中定义
| mode    | 说明     |
|:--|:--|
| S_ISUID       | 设置SUID权限(执行时设置UID)       |
| S_ISGID       | 设置SGID权限(执行时设置GID)       |
| S_ISVTX       | 设置黏着位       |
| S_IRWXU       | 所有者RWX       |
| S_IRUSR       | 所有者R       |
| S_IWUSR       | 所有者W       |
| S_IXUSR       | 所有者X       |
| S_IRWXG       | group RWX       |
| S_IRGRP       | group R       |
| S_IWGRP       | group W       |
| S_IXGRP       | group X       |
| S_IRWXO       | other RWX       |
| S_IROTH       | other R       |
| S_IWOTH       | other W       |
| S_IXOTH       | other X       |

### 黏着位 S_ISVTX
S_ISVTX实际上是save text bit(svtx bit)的缩写
- 对于早期的UNIX系统，如果二进制文件的svtx位被设置，该文件被执行后，其正文段仍然保存在交换区。该机制现已经被虚拟内存机制取代
>备注：一个二进制文件由 正文段、数据段、bss段构成

- 如果一个目录的svtx位被设置，那么只有目录的所有者和root能够删除或重命名其中的文件

## chown, fchown, lchown 和 fchownat 函数
```c
#include <sys/stat.h>
int chown(const char *path, uid_t owner, gid_t group);
int fchown(int fildes, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group);
int fchownat(int fd, const char *path, uid_t owner, gid_t group, int flag);
返回值: 成功返回0，失败返回-1
```
- owner 和 group 参数如果是 -1，则代表对应ID不变
- 当文件是符号链接时，fchownat函数 的参数 flag 为 AT_SYMLINK_NOFOLLOW时，等价与lchown，都是修改符号链接本身的拥有者
- 文件拥有者可以将组改为拥有者的其他附属组，但是只有root用户才能改变文件拥有者
>备注：如果 fchownat 的 path 指定相对路径，fd 参数指定为 AT_FDCWD 那么，路径名就在当前工作目录下获取

## 文件长度
stat 结构体的 st_size 用于表示以字节为单位的文件长度
- 只对 普通文件、目录文件、管道文件、符号链接 有意义
>备注：文件空洞形成的原因：
stat结构体中的st_blksize和st_blocks属性是关于文件在文件系统中占据空间的属性，第一个是最优文件系统块大小，第二个是分配给该文件的块数。文件空洞的实质就是文件长度小于所占用的磁盘块大小。当使用cat命令复制带空洞的文件时，新的文件就不存在空洞了，因为原先的空洞被null(0字节)填满

## truncate 和 ftruncate
截断文件
```c
#include <unistd.h>
int truncate(const char *path, off_t length);
int ftruncate(int fildes, off_t length);
返回值:成功返回0，失败返回-1
```
这两个函数可以将文件截断也可以扩展，如果length大于文件长度，那么就会扩展当前的文件，并且扩展的空间将以文件空洞的形式存在。

而且注意，这两个函数不会修改当前文件的偏移量，如果不注意这一点，很有可能造成偏移量超出了新的文件长度，从而形成空洞。

## 文件系统
- Unix的链接分为两种，硬链接和软链接(符号链接)，每个inode块都有链接计数，只有当链接计数为0的时候，文件系统才能的删除数据块
- 符号链接实际上是一种文件(有着自己的索引块和数据块)，软连接的存在不会让索引计数器改变，只是数据块内容时另一个文件的位置。符号链接能跨文件系统
- 硬链接是文件系统inode块产生的，链接其他文件的block块，所以硬链接无法跨文件系统
- 一个新目录的链接计数必然是2，因为目录下默认就有两个特殊文件.和..，目录中的每一个子目录都会让父目录的链接计数增1

## link linkat unlink unlinkat
用于创建和释放链接
```c
#include <unistd.h>
int link(const char *path1, const char *path2);
int linkat(int fd1, const char *name1, int fd2, const char *name2, int flag);

int unlink(const char *path);
int unlinkat(int fd, const char *path, int flag);
返回值：成功返回0，失败返回-1
```
- fd 可以用 AT_FDCWD 代替，这代表着路径将以当前工作目录来计算
- linkat函数的flag参数只有AT_SYMLINK_FOLLOW可用，当这个参数存在时，函数将创建指向链接目标的链接，否则创建指向符号链接本身的链接
- unlinkat的flag则只有AT_REMOVEDIR可用，当此参数被传入时，unlinkat将和rmdir一样删除目录。这个特性可以用来保证进程崩溃时，删除临时文件
>备注 ISO C标准库函数remove同样可以用来接触链接

## rename 和 renameat 函数
重命名文件或路径
```c
#include <stdio.h>
int rename(const char *old, const char *new);
int renameat(int fromfd, const char *from, int tofd, const char *to);
返回值:成功返回0，失败返回-1
```
- 如果 old 指定的是文件，rename 将重命名这个文件或符号链接
> 备注:
> 1. 如果 old 指定的是文件，那么 new 就不能是目录；如果 new 也存在，先删除 new，然后将 old 重命名为 new

- 如果 old 指定的时路径，rename 将重命名这个路径
> 备注:
> 1. 如果 old 指定的是路径，那么 new 就不能是文件；如果 new 也存在，并且为空路径的时候，就删除它。
> 2. 重命名路径的时候，new 不能为 old+string，比如，不能将 /usr/foo 重命名为 /usr/foo/dir
> 3. 程序使用者需要同时具有对原路径和目标路径的写权限

- 如果 old 或 new 是 符号链接，那么 rename 只处理符号链接本身，而不处理被链接的文件

- 如果 old 和 new 相同，那么函数直接返回成功，而不做任何处理

- renameat 中的 from 和 to 如果指定的是相对路径，那么将使用 oldfd 和 newfd 计算绝对路径。 oldfd 和 newfd 可以设置为 AT_FDCWD 作为当前工作路径

## 符号链接

### symlink symlinkat
创建符号链接
```c
#include <unistd.h>
int symlink(const char *path1, const char *path2);
int symlinkat(const char *name1, int fd, const char *name2);
返回值:成功返回0，失败返回-1
```
- 为path1文件创建符号链接path2
- fd 用来计算绝对路径，当然可以设置为AT_FDCWD

### readlink readlinkat
打开被链接的文件
```c
#include <unistd.h>
ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize);
ssize_t readlinkat(int fd, const char *restrict path, char *restrict buf, size_t bufsize);
返回值:成功返回读取到的字节数，失败返回-1
```
- buf 用来缓存读取到的字节
- fd 用来计算绝对路径，当然可以设置为AT_FDCWD

## 文件的时间
UNIX系统下文件的时间由三种
| 字段     | 说明     | 例子     | ls(1)选项     |
|:--|:--|:--|:--|
| st_atime       | 文件的最后访问时间       | read       | -u       |
| st_mtime       | 文件的最后修改时间       | write       | 默认       |
| st_ctime       | inode块状态的最后更改时间       | chmod、chown       | -c       |

- 任何修改目录内容的操作也必定会导致目录的inode块被修改，即mtime修改同时ctime修改

- 任何创建子目录的行为都会导致父目录被更改，这点很好理解，因为子目录必定会导致父目录索引数目增加

- 目录下项目数目的增加都会导致目录被更改

- 重命名一个文件实际就是修改了inode块，必定会导致目录修改

## utimes utimensat futimens
```c
#include <sys/stat.h>
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2], int flag);

#include <sys/time.h>
int utimes(const char *path, const struct timeval times[2]);
```
- time数组长度为2，分别设置访问时间和修改时间
- 如果 times 参数是个空指针，则将访问时间和修改时间设置为当前时间
- 如果 tv_nsec 设置为 UTIME_OMIT, 时间戳将不会改变，tv_sec 值会被忽略
- 如果 tv_nsec 设置为 UTIME_NOW 或 UTIME_OMIT 以外的值，时间戳被设置为tv_sec 和 tv_nsec 指定的值
- utimensat 函数的 flag 参数可以指定函数的行为，比如，若被操作的文件是符号链接，当flag为AT_SYMLINK_NOFOLLOW，则只修改符号链接的时间戳，如果不设置，则会修改符号链接所指项的文件的时间戳
- 如果 utimensat 的 path 指定相对路径，fd 参数指定为 AT_FDCWD 那么，路径名就在当前工作目录下获取
### timeval 结构体
和 timespec结构体差不多，差别是 tv_usec 单位是 微秒
```c
_STRUCT_TIMEVAL
{
        time_t         tv_sec;         /* seconds */
        lang    tv_usec;        /* and microseconds */
};
```

## mkdir mkdirat rmdir
创建、删除空目录
```c
#include <sys/stat.h>
int mkdir(const char *path, mode_t mode);
int mkdirat(int fd, const char *path, mode_t mode);

#include <unistd.h>
int rmdir(const char *path);
返回值：成功返回0，失败返回-1
```
- mkdir函数对mode超出低九位的行为是没有反应的，所以最好在mkdir后再使用chmod设置权限
- 如果 mkdirat 的 path 指定相对路径，fd 参数指定为 AT_FDCWD 那么，路径名就在当前工作目录下获取

## opendir fdopendir readdir telldir seekdir rewinddir closedir
打开、读取、关闭目录
```c
#include <dirent.h>
DIR *opendir(const char *filename);
DIR *fdopendir(int fd);
返回值:成功返回DIR指针，失败返回null

struct dirent *readdir(DIR *dirp);
返回值:成功返回下一个dirent指针，路径末尾或错误返回null

long telldir(DIR *dirp); /*查询偏移量*/
返回值:dir的偏移量

int closedir(DIR *dirp);
返回值:成功返回0，失败返回-1

void seekdir(DIR *dirp, long loc); /*设置偏移量*/
void rewinddir(DIR *dirp); /*初始化偏移量*/
```

### DIR 结构体
```c
struct __dirstream {
        void *__fd; /* file descriptor associated with directory */
        char *__data;
        int __entry_data;
        char *__ptr;
        int __entry_ptr;
        size_t __allocation;
        size_t __size;
        __libc_lock_define (, __lock)
};
typedef struct __dirstream DIR;

```

### dirent 结构体
```c
typedef struct dirent
{
        long d_ino;              /* inode number 索引节点号 */
        off_t d_off;             /* offset to this dirent 在目录文件中的偏移 */
        unsigned short d_reclen; /* length of this d_name 文件名长 */
        unsigned char d_type;     /* the type of d_name 文件类型 */
        char d_name [NAME_MAX+1]; /* file name (null-terminated) 文件名，最长255字符 */
}
```

## chdir fchdir getcwd getwd
改变 获取当前工作目录
```c
#include <unistd.h>
int chdir(const char *path);
int fchdir(int fildes);
返回值:成功返回0，失败返回-1
char *getcwd(char *buf, size_t size);
char *getwd(char *buf);
返回值：成功返回buf内容，失败返回null
```

## 设备特殊文件 st_dev st_rdev
- st_dev 的值 是文件系统的设备号
- st_rdev 只有字符设备和块设备才有，是实际设备的设备号
- 可以使用 major 和 minor 这两个宏来获取主次设备号
```c
#include <sys/mkdir.h>
#include <sys/stat.h>

struct stat buf;

stat(file,&buf);

major(buf.st_dev);
minor(buf.st_dev);

major(buf.st_rdev);
minor(buf.st_rdev);
```
