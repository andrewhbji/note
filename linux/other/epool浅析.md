# epoll 浅析

## 1. 什么是  epoll

epoll 是 Linux 下多路复用 IO 接口 select/poll 的增强版本。

相比 select/poll ，提升了程序在大量并发连接中只有少量活跃的情况下的系统 CPU 利用率：

- 它会复用文件描述符集合来传递结果而不用迫使开发者每次等待事件之前都必须重新准备要被侦听的文件描述符集合。
- 获取到事件的时候，它无须遍历整个被侦听的描述符 集，只要遍历那些被内核 IO 事件异步唤醒而加入 Ready 队列的描述符集合就行了。

epoll 除了提供 select/poll 那种 IO 事件的电平触发 （ Level Triggered ）外，还提供了边沿触发（ Edge Triggered ），这就使得用户空间程序有可能缓存IO状态，减少 epoll_wait/epoll_pwait 的调用，提高应用程序效率。

这些个使我们开发网络应用程序更加简单，并且更加高效。

Linux2.6 内核中对 /dev/epoll 设备的访问的封装（ system epoll ）。

## 2. 为什么要用 epoll

在linux系统下，影响效率的依然是I/O操作，使用 epoll 的原因如下：

1. 文件描述符数量的对比

    - epoll 并没有fd(文件描述符)的上限，它只跟系统内存有关。可使用如下命令查看当前系统支持的数量：
    
    ```sh
    $ cat /proc/sys/fs/file-max
    1213152
    ```
    
    - select/poll，有一个限定的 fd 的数量，在 linux/posix_types.h 头文件中定义
    ```c
    #define __FD_SETSIZE    1024
    ```

2. 效率对比

    - select/poll会因为监听fd的数量而导致效率低下，因为它是轮询所有fd，有数据就处理，没数据就跳过，所以fd的数量会降低效率。

    - 而epoll只处理就绪的fd，它有一个就绪设备的队列，每次只轮询该队列的数据，然后进行处理。

3. 内存处理方式对比。

    - select/poll 无法回避 fd 在操作过程中的拷贝问题。
    
    - epoll 使用了 mmap (是指文件/对象的内存映射，被映射到多个内存页上)，所以同一块内存就可以避免这个问题。
    
    >备注：TCP/IP 协议栈使用内存池管理 sk_buff 结构，还可以通过修改内存池 pool 的大小。

## 3. epoll 的工作方式

epoll 分为两种工作方式 LT 和 ET ：

- LT ( level triggered ) 是默认/缺省的工作方式，同时支持 block 和 no_block socket 。这种工作方式下，内核会通知你一个 fd 是否就绪，然后才可以对这个就绪的 fd 进行 I/O 操作。就算你没有任何操作，系统还是会继续提示 fd 已经就绪，不过这种工作方式出错会比较小，传统的 select/poll 就是这种工作方式的代表。

- ET( edge-triggered ) 是高速工作方式，仅支持 no_block socket ，这种工作方式下，当 fd 从未就绪变为就绪时，内核会通知 fd 已经就绪，并且内核认为你知道该 fd 已经就绪，不会再次通知了，除非因为某些操作导致 fd 就绪状态发生变化。如果一直不对这个 fd 进行 I/O 操作，导致 fd 变为未就绪时，内核同样不会发送更多的通知，因为 only once 。所以这种方式下，出错率比较高，需要增加一些检测程序。

LT 可以理解为水平触发，只要有数据可以读，不管怎样都会通知。而 ET 为边缘触发，只有状态发生变化时才会通知，可以理解为电平变化。

## 4. epoll API

Linux 下 epoll API 在 epoll.h 中定义

### 4.1 创建 epoll fd 

epoll_create() 创建一个 epoll 的实例
```c
int epoll_create(int size);
int epoll_create1(int flag);
```
- 返回值：当创建成功后，会占用一个 epfd

- size 指定衡量内核内部结构大小的一个建议值

    >备注：自从 Linux2.6.8 版本以后， size 值其实是没什么用的，不过要大于 0，因为内核可以动态的分配大小。

- flag 可取值如下：

    - 0  表示和 epoll_create 函数完全一样
    - EPOLL_CLOEXEC  创建的 epfd 会设置 FD_CLOEXEC

    >备注：FD_CLOEXEC 用来设置文件 close-on-exec 状态的。当 close-on-exec 状态为 0 时，调用 exec 时， fd 不会被关闭；状态非零时则会被关闭，这样做可以防止 fd 泄露给执行 exec 后的进程。
    
    - EPOLL_NONBLOCK  创建的 epfd 会设置为非阻塞

### 4.2 注册 epoll 事件

和 select 不同， epoll 需要在监听事件前 使用 epoll_ctl 函数注册需要监听的事件，select 是在监听时告诉内核要监听的事件。

```c
int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
```
- epfd 是 epoll_create 返回的的epoll fd 。

- op 可取值如下：

    - EPOLL_CTL_ADD 注册目标 fd 到 epfd 中，同时关联内部 event 到 fd 上
    - EPOLL_CTL_MOD 修改已经注册到 fd 的监听事件
    - EPOLL_CTL_DEL 从 epfd 中删除/移除已注册的 fd ， event 可以被忽略，也可以为 NULL

- fd 表示需要监听的 fd ，需要监听 socket 则传入 socket fd

- event 指针指向 epoll_event 结构体

epoll_event 结构体定义如下：
```c
typedef union epoll_data {
    void        *ptr;
    int          fd;
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;      /* Epoll events */
    epoll_data_t data;        /* User data variable */
};
```
- events 成员用来记录需要监听事件类型，可以使用 “|” 的方式设置多个类型值，可取值如下

    - EPOLLIN: 表示关联的fd可以进行读操作了。
    - EPOLLOUT: 表示关联的fd可以进行写操作了。
    - EPOLLRDHUP(since Linux 2.6.17): 表示套接字关闭了连接，或者关闭了正写一半的连接。
    - EPOLLPRI: 表示关联的fd有紧急优先事件可以进行读操作了。
    - EPOLLERR: 表示关联的 fd 发生了错误，epoll_wait 会一直等待这个事件，所以一般没必要设置这个属性。
    - EPOLLHUP: 表示关联的 fd 挂起了， epoll_wait 会一直等待这个事件，所以一般没必要设置这个属性。
    - EPOLLET: 设置关联的 fd 为 ET 的工作方式， epoll 的默认工作方式是 LT。
    - EPOLLONESHOT (since Linux 2.6.2): 设置关联的fd为one-shot的工作方式。表示只监听一次事件，如果要再次监听，需要把socket放入到epoll队列中。
    
- data 可以指向一些用户数据，比如指向回调函数。

### 4.3 等待 epoll 事件

```c
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout,  const sigset_t *sigmask);
```
- 返回值：捕获到事件则返回就绪的 fd， 超时则返回 0， 出错则返回 errno

- epfd 表示 epoll_wait 等待 epfd 上的事件

- events 指针携带有 epoll_data_t 数据

- maxevents 告诉内核 events 有多大，该值必须大于 0

- timeout 表示超时时间

- sigmask 表示要捕获的信号量

    
    >备注: epoll_pwait(since linux 2.6.19)允许一个应用程序安全的等待，直到fd设备准备就绪，或者捕获到一个信号量。
    
## 5.1 例子

### 5.2 客户端
```cpp
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAXDATASIZE 100 // 每次可发送的最大字节数

int main(int argc, char *argv[]){
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he; // 用于设置 server 端信息
    struct sockaddr_in their_addr; // 用于设置连接信息

    if (( argc == 1) || (argc==2) ){
        fprintf(stderr,"usage: client hostname\nEx:\n$./client01 ip port\n");
        exit(1);
    }

    // 获取服务器 host 信息
    if ((he=gethostbyname(argv[1])) == NULL){ // get the host info
        herror("gethostbyname");
        exit(1);
    }

    // 分配一个 socket
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET; // host byte order
    their_addr.sin_port = htons(atoi(argv[2])); // 设置端口
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);  // 设置地址
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

    // 连接请求
    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof their_addr) == -1){
        perror("connect");
        exit(1);
    }

    while( 1 ){
        // 发送数据
        if(send(sockfd, "hello, this is client message!", strlen("hello, this is client message!"), 0 ) == -1){
            perror("send");
        }

        // 接受服务器返回消息
        if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1){
            perror("recv");
        }

        else if( numbytes == 0 ){
            printf("服务器已关闭!\n");
            break;
        }

        // 消息末尾追加 \0
        buf[numbytes] = '\0';

        // 打印消息
        printf("Received: %s \n",buf);
        sleep(1);
    }

    // 断开连接
    close(sockfd);
    return 0;
}
```
### 5.3 服务端
```cpp
//
// a simple echo server using epoll in linux
//
// 2009-11-05
// by sparkling
//
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <string.h>

using namespace std;
#define MAX_EVENTS 500

struct myevent_s{
    int fd;
    void (*call_back)(int fd, int events, void *arg);
    int events;
    void *arg;
    int status; // 1: in epoll wait list, 0 not in
    char buff[128]; // recv data buffer
    int len;
    long last_active; // last active time
};

// set event
void EventSet(myevent_s *ev, int fd, void (*call_back)(int, int, void*), void *arg){
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    ev->last_active = time(NULL);
}

// add/mod an event to epoll
void EventAdd(int epollFd, int events, myevent_s *ev){
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;  // myevent_s 结构体实例作为 epoll_event 结构体实例的数据域
    epv.events = ev->events = events;   // 设置监听的事件类型
    if(ev->status == 1){
        op = EPOLL_CTL_MOD;
    }
    else{
        op = EPOLL_CTL_ADD;
        ev->status = 1;     // myevent_s.status = 1 表示事件在 epoll 等待队列中中
    }
    if(epoll_ctl(epollFd, op, ev->fd, &epv) < 0)
        printf("Event Add failed[fd=%d]\n", ev->fd);
    else
        printf("Event Add OK[fd=%d]\n", ev->fd);
}

// delete an event from epoll
void EventDel(int epollFd, myevent_s *ev){
    struct epoll_event epv = {0, {0}};
    if(ev->status != 1) return;
    epv.data.ptr = ev;
    ev->status = 0;     // myevent_s 结构体实例回复为可用
    epoll_ctl(epollFd, EPOLL_CTL_DEL, ev->fd, &epv);
}

int g_epollFd;
myevent_s g_Events[MAX_EVENTS+1]; // g_Events[MAX_EVENTS] is used by listen fd
void RecvData(int fd, int events, void *arg);
void SendData(int fd, int events, void *arg);

// 处理客户端的连接请求
void AcceptConn(int fd, int events, void *arg){
    struct sockaddr_in sin;
    socklen_t len = sizeof(struct sockaddr_in);
    int nfd, i;

    // 1. 接受客户端的连接请求，并返回一个新的 fd 用于接收客户端发送的消息
    if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) == -1){
        if(errno != EAGAIN && errno != EINTR){
            printf("%s: bad accept", __func__);
        }
        return;
    }

    // 2. 准备接收客户端消息
    do{
        //取一个可用的 myevent_s 结构体实例
        for(i = 0; i < MAX_EVENTS; i++){
            if(g_Events[i].status == 0){
                break;
            }
        }
        // 可用结构体实例最大数为 100，用完了则报错
        if(i == MAX_EVENTS){
            printf("%s:max connection limit[%d].", __func__, MAX_EVENTS);
            break;
        }
        // 将连接设置为非阻塞
        if(fcntl(nfd, F_SETFL, O_NONBLOCK) < 0) break;

        // 设置收到客户端消息的事件
        EventSet(&g_Events[i], nfd, RecvData, &g_Events[i]);
        EventAdd(g_epollFd, EPOLLIN|EPOLLET, &g_Events[i]);
        printf("new conn[%s:%d][time:%d]\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), g_Events[i].last_active);
    }while(0);
}

// 处理客户端发送的消息
void RecvData(int fd, int events, void *arg){
    struct myevent_s *ev = (struct myevent_s*)arg;
    int len;
    // 1. 收取消息
    len = recv(fd, ev->buff, sizeof(ev->buff)-1, 0);
    EventDel(g_epollFd, ev);    // 收取完消息后，将用于接收客户端消息的 fd 从 epoll 的等待队列中丢弃，将 ev.status 重置为 0
    if(len > 0){
        ev->len = len;
        ev->buff[len] = '/0';
        printf("C[%d]:%s\n", fd, ev->buff);

        // 2. 继续使用 ev 和刚才的 fd 设置发送消息的事件
        EventSet(ev, fd, SendData, ev);
        EventAdd(g_epollFd, EPOLLOUT|EPOLLET, ev);
    }
    else if(len == 0){
        // 不发送消息，则关闭用于接收消息（也是发送消息）的 fd
        close(ev->fd);
        printf("[fd=%d] closed gracefully.\n", fd);
    }
    else{
        // 接收到错误消息，则关闭用于接收消息（也是发送消息）的 fd
        close(ev->fd);
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }
}

// 发送消息回调函数
void SendData(int fd, int events, void *arg){
    struct myevent_s *ev = (struct myevent_s*)arg;
    int len;
    // 发送消息
    len = send(fd, ev->buff, ev->len, 0);
    ev->len = 0;
    EventDel(g_epollFd, ev);    // 发送完消息后，将用于发送消息的 fd 从 epoll 的等待队列中丢弃，将 ev.status 重置为 0
    if(len > 0){
        // 设置继续等待接受客户端消息
        EventSet(ev, fd, RecvData, ev);
        EventAdd(g_epollFd, EPOLLIN|EPOLLET, ev);
    }
    else{
        // 不等待客户端消息，则关闭用于发送消息（也是接收消息）的 fd
        close(ev->fd);
        printf("recv[fd=%d] error[%d]\n", fd, errno);
    }
}

void InitListenSocket(int epollFd, short port){
    // 1. 分配 socket
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(listenFd, F_SETFL, O_NONBLOCK); // 为 socket 设置非阻塞属性
    printf("server listen fd=%d\n", listenFd);

    // 2. 设置监听客户端连接请求事件
    EventSet(&g_Events[MAX_EVENTS], listenFd, AcceptConn, &g_Events[MAX_EVENTS]);
    // add listen socket
    EventAdd(epollFd, EPOLLIN|EPOLLET, &g_Events[MAX_EVENTS]);

    // 3. bind & listen socket
    sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    bind(listenFd, (const sockaddr*)&sin, sizeof(sin));
    listen(listenFd, 5);    // 设置 socket 的队列中最多可以有5个 pending 的连接请求
}

int main(int argc, char **argv){
    short port = 12345; // default port
    if(argc == 2){
        port = atoi(argv[1]);
    }

    // 创建 epoll
    g_epollFd = epoll_create(MAX_EVENTS);
    if(g_epollFd <= 0) printf("create epoll failed.%d\n", g_epollFd);

    // create & bind listen socket, and add to epoll, set non-blocking
    InitListenSocket(g_epollFd, port);

    // event loop
    struct epoll_event events[MAX_EVENTS];
    printf("server running:port[%d]\n", port);
    int checkPos = 0;

    // 进入轮询
    while(1){
        // a simple timeout check here, every time 100, better to use a mini-heap, and add timer event
        long now = time(NULL);
        for(int i = 0; i < 100; i++, checkPos++) // doesn't check listen fd
        {
            if(checkPos == MAX_EVENTS) checkPos = 0; // recycle
            if(g_Events[checkPos].status != 1) continue;
            long duration = now - g_Events[checkPos].last_active;
            if(duration >= 60) // 丢弃 60s timeout 的连接请求和消息
            {
                close(g_Events[checkPos].fd);   // 关闭已经超时的连接
                printf("[fd=%d] timeout[%d--%d].\n", g_Events[checkPos].fd, g_Events[checkPos].last_active, now);
                EventDel(g_epollFd, &g_Events[checkPos]);   // 将超时的连接从 epoll 的等待队列中移除
            }
        }
        // 进入等待状态
        int fds = epoll_wait(g_epollFd, events, MAX_EVENTS, 1000);
        if(fds < 0){
            printf("epoll_wait error, exit\n");
            break;
        }
        for(int i = 0; i < fds; i++){
            myevent_s *ev = (struct myevent_s*)events[i].data.ptr;
            if((events[i].events&EPOLLIN)&&(ev->events&EPOLLIN)) // 处理接受客户端连接请求、以及接收客户端消息事件
            {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
            if((events[i].events&EPOLLOUT)&&(ev->events&EPOLLOUT)) // 处理向客户端发送消息事件
            {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }
    // free resource
    return 0;
}
```