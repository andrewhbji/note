/*
 * =====================================================================================
 *
 *       Filename:  test.cpp
 *
 *    Description:  reliable and unreliable signal test
 *
 *        Version:  1.0
 *        Created:  2012年05月07日 22时52分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  LeoK,
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

int g_unreliable = 0;
int g_reliable = 0;
void signal_dispath(int signo)
{
    if(signo == SIGUSR1)
    {
        printf("receving signal SIGUSR1\n");
        g_unreliable++;
    }
    else if(signo == SIGRTMIN)
    {
        printf("receving signal SIGRTMIN\n");
        g_reliable++;
    }
}
void output_myself()
{
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("^^^^^^^ Hello World ^^^^^^^\n");
    printf("^^^^^^^ I'm  LeoK   ^^^^^^^\n");
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

int main(int argc, char** argv)
{
    if(argc != 1)
    {
        printf("this program need not paramter\n");
        return 0;
    }
    /* 每个程序的信息需要打印*/
    output_myself();
    if(signal(SIGUSR1, signal_dispath) == SIG_ERR)
    {
        perror("register SIGUSR1 signal failed");
        return -1;
    }
    if(signal(SIGRTMIN, signal_dispath) == SIG_ERR)
    {
        perror("register SIGRTMIN signal failed");
        return -1;
    }

    sigset_t set;
    sigset_t oset;
    /*  清除set中的信号，把这个set置为空 */
    sigemptyset(&set);
    /*  往set中添加SIGUSR1信号 */
    sigaddset(&set, SIGUSR1);
    /*  往set中添加SIGRTMIN信号 */
    sigaddset(&set, SIGRTMIN);
    if(sigprocmask(SIG_BLOCK, &set, &oset) == -1)
    {
        perror("set process signal to be set failed");
        return -1;
    }
    sleep(10);
    if(sigpending(&set) == -1)
    {
        perror("sigpending get signal mask failed");
        return -1;
    }
    /*  判断信号是不是在信号屏蔽字中 */
    /*  SIGUSR1 */
    int ret = -1;
    if((ret = sigismember(&set, SIGUSR1)) == -1)
    {
        perror("sigismember test SIGUSR1 failed");
        return -1;
    }
    else if(ret == 0)
    {
        printf("SIGUSR1 signal exist not signal mask\n");
    }
    else
    {
        printf("SIGUSR1 signal exist signal mask\n");
    }

    /*  SIGRTMIN */
    if((ret = sigismember(&set, SIGRTMIN)) == -1)
    {
        perror("sigismember test SIGRTMIN failed");
        return -1;
    }
    else if(ret == 0)
    {
        printf("SIGRTMIN signal exist not signal mask\n");
    }
    else
    {
        printf("SIGRTMIN signal exist signal mask\n");
    }
    /*  休眠100s */
    sleep(30);
    if(sigprocmask(SIG_SETMASK, &oset, NULL) == -1)
    {
        perror("set oldset failed");
        return -1;
    }
    printf("SIGUSR1:      %d\n",&g_unreliable);
    printf("SIGRTMIN:     %d\n",&g_reliable);
    output_myself();
    return 0;
}

