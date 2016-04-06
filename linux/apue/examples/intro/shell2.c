#include "apue.h"
#include <sys/wait.h>

/*声明信号处理函数*/
static void    sig_int(int);

int
main(void)
{
    char    buf[MAXLINE];    /*MAXLINE 在 apue.h 中声明，常量*/
    pid_t    pid;
    int        status;


    /*signal函数, 指定SIGINT 到处理函数 sig_int*/
    /*机制, 类似于直接注册到了进程, 观察是否异常发生后捕获处理*/
    if (signal(SIGINT, sig_int) == SIG_ERR)
        err_sys("signal error");

    printf("%% ");    /*打印提示符 (打印 % 需要 %% )*/

    while (fgets(buf, MAXLINE, stdin) != NULL) {
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0; /*newline 替换为 null*/

        if ((pid = fork()) < 0) {
            err_sys("fork error");
        } else if (pid == 0) {        /*child*/
            execlp(buf, buf, (char *)0);
            err_ret("couldn't execute: %s", buf);
            exit(127);
        }

        /*parent*/
        if ((pid = waitpid(pid, &status, 0)) < 0)
            err_sys("waitpid error");
        printf("%% ");
    }
    exit(0);
}


/*信号处理函数, 当检测到中断键输入时，打印*/
void
sig_int(int signo)
{
    printf("interrupt\n%% ");
}
