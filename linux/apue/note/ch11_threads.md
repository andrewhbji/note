[TOC]
# 第11章 线程

## 进程和线程的关系
- 线程包含了进程内执行环境的必须信息，包括线程ID、一组寄存器值、栈、调度优先级和策略、信号掩码、errno变量以及线程私有数据
- 线程共享进程的所有内容，包括正文段、全局内存、栈、堆和文件描述符
- 进程在创建的时候被看成只有一个控制线程，这时进程只能执行一个任务。有了线程机制，一个进程就可以并行执行多个任务，有利于简化处理异步事件的代码，改善程序吞吐量，改善程序响应时间，有效利用cpu

## 线程标识
UNIX 提供 phread_t 数据类型作为线程ID的存储类型，不同的UNIX 环境的phread_t实现不同，因此为保证可移植性，尽量不去直接操作这个数据类型，而是根据当前系统来确定pthread_t

### phread_equal phread_self
UNIX 提供 pthread_self 函数获取当前线程的tid，pthread_equal 函数用于比较两个tid是否相同
```c
#include <pthread.h>
int pthread_equal(pthread_t tid1, pthread_t tid2);
返回值：如果两个tid相同，返回非0值，否则返回0
pthread_t pthread_self(void);
返回当前线程的tid
```

## 线程创建
UNIX 使用 pthread_create 函数创建线程
```c
#include <pthread.h>
int pthread_create(pthread_t *restrict tidp, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg);
返回值：成功返回0，失败返回错误码
```
- 如果pthread_create成功调用，tidp 指针指向新线程tid的地址
- attr 用来指定线程的类型，如果 attr为空，则使用默认属性
- start_routine 是个函数指针，函数本身无参数，无返回值，新线程创建后，自动执行这个函数
- arg 用于给 start_routine 传参，如果参数列表不知一个，那么就需要吧这些参数放在一个结构体中，然后把结构体的地址作为arg参数传入
- UNIX 不保证哪个线程先运行
- 新线程可以访问进程的地址空间，继承调用线程的浮点环境和信号掩码，但是不会继承调用线程的未决信号集

## 线程终止
线程由三种调用方式：
- 线程返回，返回值时线程的退出码
- 被其他线程取消
- 线程调用pthread_exit

### pthread_exit
```c
#include <pthread.h>
void pthread_exit(void *rval_ptr);
```
- rval_ptr 参数可以被pthread_join访问
```c
pthread_exit((void *)2);
```

### pthread_join
类似进程中使用的wait函数，调用线程中使用 pthread_join 会阻塞调用线程，直到 tid 指定的线程调用 pthread_exit 或 被取消。如果指定线程调用pthread_exit，rval_ptr包含线程返回码，如果指定线程被取消，rval_ptr被设置为PTHREAD_CANCELED
```c
#include <pthread.h>
int pthread_join(pthread_t tid, void **rval_ptr);
返回值：成功返回0，失败返回错误码
```
>备注：pthread_exit 的 rval_ptr 参数可以传递复杂数据类型，在传递复杂数据类型的时候，尽量不要在栈内存中分配(如局部变量)，一般使用全局变量，或使用malloc分配内存

### pthread_cancel
线程可以通过调用 pthread_cancel 来请求取消指定线程，被取消的线程如同调用了参数为 PTHREAD_CANCELED 的 pthread_exit 函数
```c
#include <pthread.h>
int pthread_cancel(pthread_t tid);
返回值：成功返回0，失败 返回错误码
```

### pthread_cleanup_push pthread_cleanup_pop
pthread_cleanup_push 用来注册线程终止时要调用的函数, arg 可以被调函数传参，被注册函数在以下三种情况下可被执行：
- 线程中调用了 pthread_exit
- 其他线程调用了 pthread_cancel
- 线程中以非0参数调用 pthread_cleanup_pop
pthread_cleanup_pop 用来清理位于栈顶的 pthread_cleanup_push 注册的函数，当execute 参数为0时，只执行清理不执行
```c
#include <pthread.h>
void pthread_cleanup_push(void (*routine)(void *), void *arg);
void pthread_cleanup_pop(int execute);
```

### pthread_detach
pthread_detach 使指定的线程处于分离态，处于分离态的线程在线程终止时立即被回收。分离态的线程不能使用pthread_join等待其终止状态，否则pthread_join的调用返回EINVAL
```c
#include <pthread.h>
int pthread_detach(pthread_t thread);
返回值：成功返回0，失败返回错误码
```

## 线程同步
线程由于共享同一个进程的内存空间，所以资源的访问需要受到限制，一个线程在读取的时候其他线程不能写入，这种限制被称为同步机制。同步机制有五种：互斥锁、读写锁、条件变量、自旋锁、屏障锁

### 互斥锁
pthread模型提供了互斥锁 pthread_mutex_t ，通过对锁定互斥锁，使同一时间只允许一个线程访问资源。当锁定互斥锁后，任何试图锁定互斥锁的线程都会被阻塞，直到当前线程解锁互斥锁

互斥锁在使用前，必须进行初始化，我们也可以将其设置为常量 PTHREAD_MUTEX_INITIALIZER

#### pthread_mutex_init pthread_mutex_destroy
也可以使用 pthread_mutex_init 创建互斥锁，pthread_mutex_destroy 用于销毁 pthread_mutex_init初始化的互斥锁
```c
#include <pthread.h>
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
返回值：成功返回0，失败返回错误码
```
- attr参数用于初始化互斥锁的属性，如果attr为null，则使用默认的属性参数

#### 互斥锁的使用
- pthread_mutex_lock 函数用于锁定互斥锁，其他线程执行到这里时会被阻塞，直到调用线程解锁互斥锁；
- pthread_mutex_unlock 函数用于解锁互斥锁
- pthread_mutex_trylock 函数用于尝试锁定互斥锁，如果互斥锁处于解锁状态，则为其锁定并阻塞其他线程，否则返回EBUSY并且不用阻塞其他进程
```c
#include <pthread.h>
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
返回值：成功返回0，失败返回错误码
```

#### 避免死锁
一个线程试图对同一个互斥量加锁两次，那么它自身会陷入死锁状态，再比如两个线程都在等待对方已经占有的资源，也会导致死锁，所以尽量使用pthread_mutex_trylock来避免死锁

#### pthread_mutex_timedlock
pthread_mutex_timedlock 给锁定加上超时机制，到指定时间后自动解锁 pthread_mutex_t
```c
#include <pthread.h>
#include <time.h>
int pthread_mutex_timedlock(pthread_mutex_t *restrict mutex, const struct timespec *restrict tsptr);
返回值：成功返回0，失败返回错误码
```

### 读写锁
读写锁pthread_rwlock_t是一个更加灵活的锁机制，有三种状态，读锁定、写锁定、解除锁定；资源被读锁定时，所有尝试进行写锁定的线程都会被阻塞；资源被写锁定时，所有尝试锁定的线程都会被阻塞；多个线程可以对资源读锁定，只有一个线程可以写锁定

pthread_rwlock_init 用于创建pthread_rwlock_t互斥锁，pthread_rwlock_destroy 用于销毁pthread_rwlock_t互斥锁
```c
#include <pthread.h>
int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
返回值：成功返回0，失败返回错误码
```

pthread_rwlock_rdlock 读锁定 pthread_rwlock_t，pthread_rwlock_wrlock 写锁定 pthread_rwlock_t，pthread_rwlock_tryrdlock 尝试读锁定 pthread_rwlock_t，pthread_rwlock_trywrlock 尝试写锁定 pthread_rwlock_t，pthread_rwlock_unlock 用于解锁 pthread_rwlock_t
```c
#include <pthread.h>
int pthread_rwlock_rdlock(pthread_rwlock_t *lock);
int pthread_rwlock_wrlock(pthread_rwlock_t *lock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *lock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *lock);
int pthread_rwlock_unlock(pthread_rwlock_t *lock);
返回值：成功返回0，失败返回错误码
```
- 对于已经写锁定的资源，任何调用 pthread_rwlock_rdlock 和 pthread_rwlock_wrlock 的其他线程都会被阻塞；
- 对于已经读锁定的资源，调用 pthread_rwlock_wrlock 的其他线程会被阻塞，调用 pthread_rwlock_rdlock 的其他线程仍可以锁定资源
- 对于已经写锁定的资源，其他线程调用 pthread_rwlock_trywrlock 和 pthread_rwlock_tryrdlock 都返回EBUSY；
- 对于已经读锁定的资源，其他线程调用 pthread_rwlock_trywrlock 都返回EBUSY，其他线程调用 pthread_rwlock_tryrdlock 的仍可以锁定资源

### 带有超时的读写锁
对读写锁加入超时机制，到期自动解锁
```c
#include <pthread.h>
int pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rwlock, const struct *timespec *restrict tsptr);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rwlock, const struct *timespec *restrict tsptr);
返回值：成功返回0，失败返回错误码
```

### 条件变量
条件变量是利用线程间共享的全局变量进行同步的一种机制，主要包括两个动作：一个线程阻塞，直到条件成立；另一个线程在条件成立时发出信号
条件变量 pthread_cond_t 一般和 pthread_mutex_t 配合使用，防止条件成立时多个线程竞争资源

pthread_cond_t 是受 pthread_mutex_t 保护的，线程在条件改变前，需先将 pthread_mutex_t 锁定

#### pthread_cond_init pthread_cond_destroy
pthread_cond_t 可以初始化为 PTHREAD_COND_INITIALIZER，同样也可以使用 pthread_cond_init 创建 pthread_cond_t；pthread_cond_destroy 用于 销毁 pthread_cond_t
```c
#include <pthread.h>
int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);
int pthread_cond_destroy(pthread_cond_t *cond);
返回值：成功返回0，失败返回错误编号
```

#### pthread_cond_wait pthread_cond_timedwait
```c
#include <pthread.h>
int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime);
返回值：成功返回0，失败返回错误编号
```
- 条件变量机制会将所有等待条件的线程加入一个等待队列
- pthread_cond_wait 做的第一件事就是将调用线程加入等待队列，并将 pthread_mutex_t 解锁，这两个操作是原子操作；然后将调用线程阻塞，等待条件成立；条件成立后，pthread_cond_wait 会将 pthread_mutex_t 重新锁定，将线程出队，然后返回，这样，其他调用 pthread_cond_wait 的线程仍然保持阻塞状态。
- pthread_cond_timedwait 加入超时机制，过期自动解除阻塞

#### pthread_cond_signal pthread_cond_broadcast
条件成立后，线程用这两个函数通知 pthread_cond_t
- pthread_cond_signal 只会通知等待队列中的第一个线程
- pthread_cond_broadcast 会通知等待队列中的所有线程，让首先解除阻塞的线程重新锁定 pthread_mutex_t
- 一般调用 pthread_cond_signal 或 pthread_cond_broadcast 前，需要将 pthread_mutex_t 重新锁定，调用完成后将之解锁
```c
#include <pthread.h>
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
返回值：成功返回0，失败返回错误编号
```

### 自旋锁
自旋锁机制和互斥锁机制的原理和使用基本一样，不同的是，如果自旋锁 pthread_spinlock_t 已经被锁定，其他申请资源的线程会一直循环查看pthread_spinlock_t 是否被解锁。相较于互斥锁，其短时间内自旋锁的性能占优，随着执行时间的延长，互斥锁的性能会超过自旋锁，并且差距会逐渐拉大，所以自旋锁比较适用于短时间锁定资源

#### pthread_spin_init pthread_spin_destroy
init 用于创建 pthread_spinlock_t，destroy 函数用于销毁 pthread_spinlock_t
```c
#include <pthread.h>
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_destroy(pthread_spinlock_t *lock);
返回值：成功返回0，失败返回错误编号
```
- pshared 参数一般被指定为 0，如果系统支持线程进程共享同步选项(由SUS定义)，可以被设置为 PTHREAD_PROCESS_SHARED 和 PTHREAD_PROCESS_PRIVATE，这两个值的区别在于是否允许自旋锁被跨进程访问

#### pthread_spin_lock pthread_spin_trylock pthread_spin_unlock
和 互斥锁 的用法基本一致
```c
#include <pthread.h>
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);
返回值：成功返回0，失败返回错误编号
```

### 屏障锁
如果某个任务需要先初始化n个线程，然后再启动这些线程，那么屏障机制是合适的选择。所有使用屏障机制的线程在就绪后都会被阻塞，直到所有线程都就绪，屏障锁自动解锁

#### pthread_barrier_init pthread_barrier_destroy
init 用户初始化屏障锁 pthread_barrier_t，destroy 用于销毁 pthread_barrier_t
```c
#include <pthread.h>
int pthread_barrier_init(pthread_barrier_t *restrict barrier, const pthread_barrierattr_t *restrict attr, unsigned int count);
int pthread_barrier_destroy(pthread_barrier_t *restrict barrier);
返回值：成功返回0，失败返回错误码
```
- count 用于指定预期的线程数

#### pthread_barrier_wait
所有调用 pthread_barrier_wait 的线程都会被阻塞在这里，当阻塞的线程数达到预期值，最后被调用 pthread_barrier_wait 返回 PTHREAD_BARRIER_SERIAL_THREAD，失败返回错误码，其他线程调用的 pthread_barrier_wait 返回 0。此时 pthread_barrier_t 会被初始化
```c
#include <pthread.h>
int pthread_barrier_wait(pthread_barrier_t *barrier);
返回值：成功返回 0 或 PTHREAD_BARRIER_SERIAL_THREAD，失败返回错误码
```
