[TOC]
# 第6章 系统数据文件和信息

## 口令文件

### passwd 结构体
pwd.h 中的 passwd 结构体对应 /etc/passwd
```c
#include <pwd.h>
struct passwd {
        char    *pw_name;               /* user name */
        char    *pw_passwd;             /* encrypted password */
        uid_t   pw_uid;                 /* user uid */
        gid_t   pw_gid;                 /* user gid */
        char    *pw_gecos;              /* Honeywell login info */
        char    *pw_dir;                /* home directory */
        char    *pw_shell;              /* default shell */
};
```

系统提供两个函数根据uid和loginname获取 passwd 结构体
```c
#include <pwd.h>
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);
返回值：成功返回指针，失败返回NULL
```
>备注：返回的结构体指针其实是指向静态区域的指针

getpwent 函数可以遍历/etc/passwd文件，并一项一项的返回passwd信息, setpwent 函数确保从头读取文件；读取结束后，需要使用endpwent() 函数
```c
#include <pwd.h>
struct passwd *getpwent(void);
返回值：成功返回指针，失败返回NULL
void setpwent(void);
void endpwent(void);
```

## 口令
口令存放在/etc/shadow中，需要root权限才能读取

spwd 结构体对应/etc/shadow
```c
struct passwd {
        char  *sp_namp        /* login name */
        char  *sp_pwdp        /* 密码 */
        int   sp_lstchg       /* 最近变动日期 */
        int   sp_min          /* 口令不可被更动的天数 */
        int   sp_max          /* 口令需要重新变更的天数 */
        int   sp_warn         /* 口令需要变更期限前的警告天数 */
        int   sp_inact        /* 口令过期后的账号宽限时间(口令失效日) */
        int   sp_expire       /* 账号失效日期 */
        unsigned int sp_flag  /* 保留 */
};
```

shadow.h 提供函数获取口令，函数的调用方式基本上和passwd一致
```c
#include <shadow.h>
struct spwd *getspnam(const char *name);
struct spwd *getspent(void);
返回值：成功返回指针，出错返回NULL
void setspent(void);
void endspent(void);
```

## 组文件
组文件存放在/etc/group 中，需要root权限才能读取

group 结构体对应/etc/group
```c
struct group {
        char    *gr_name;               /* [XBD] group name */
        char    *gr_passwd;             /* [???] group password */
        gid_t   gr_gid;                 /* [XBD] group id */
        char    **gr_mem;               /* [XBD] group members */
};
```

group.h 提供函数获取口令，函数的调用方式基本上和passwd一致
```c
#include <grp.h>
struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *groupname);

struct group *getgrent(void);
返回值：成功返回指针，出错返回NULL
void setgrent(void);
void endgrent(void);
```

## 附属组(有效组)ID
- getgroups函数将进程所属用户的附属组ID放入grouplist参数中，groupsize参数用于确定放入的大小，但是实际上，我们可以将groupsize设置为0，然后函数会返回实际的附属组个数，然后就可以很方便的分配grouplist数组，用不着去猜测究竟应该分配多少。
- setgroups是一个root权限操作，用于为进程设置附属组ID。
- initgroups是一系列操作的集合，实际上用到的机会极少，如果有需要的朋友可以自行观看系统手册。
```c
#include <grp.h>
#include <unistd.h>
int getgroups(int gidsetsize, gid_t grouplist[]);
返回值：成功返回附属组数，失败返回-1
int setgroups(int ngroups, const gid_t *gidset);
int initgroups(const char *name, int basegid);
返回值：成功返回0，失败返回-1
```

## 其他数据文件
| 描述     | 数据文件     | Header     | 结构体     | 查看函数     |
|:--|:--|:--|:--|:--|
| hosts       | /etc/hosts       | <netdb.h>  | hostent  |  getnameinfo, getaddrinfo  |
| networks       | /etc/networks       | <netdb.h>  | netent  |  getnetbyname, getnetbyaddr  |
| protocols       | /etc/protocols       | <netdb.h>  | protoent  |  getprotobyname, getprotobynumber  |
| services       | /etc/services       | <netdb.h>  | servent  |  getservbyname, getservbyport  |

## 登陆帐户信息
用来记录登陆用户信息
```c
struct utmp {
        char ut_name[8];    /* login name */
        long ut_time;        /* seconds since Epoch */
        char ut_line[8];    /* tty line: "ttyh0", "ttyd0", "ttyp0", ... */
};

```

## 系统标示
uname函数，用于返回主机和操作系统相关的信息，uname命令就是使用了这个函数
```c
int uname(struct utsname *name);

struct  utsname {
        char    sysname[];  /* [XSI] Name of OS */
        char    nodename[]; /* [XSI] Name of this network node */
        char    release[];  /* [XSI] Release level */
        char    version[];  /* [XSI] Version level */
        char    machine[];  /* [XSI] Hardware type */
};
```
BSD派生系统也提供了gethostname函数
```c
int gethostname(char *name, size_t namelen);
```

## 日期和时间
Unix系统都使用的是Unix时间戳，也就是UTC时间1970年1月1日0时0分0秒以来的秒数，在前面我们提到过它，这个就是日历时间，并且是使用time_t类型存储的
```c
time_t time(time_t *tloc);
```
time函数就是很简单的获取日历时间，在一般的情况下，都是传入null然后获得函数返回值来使用。
