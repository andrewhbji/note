#include "apue.h"
#include <sys/wait.h>

/*fork创建一个新进程, 它被调用一次(由父进程调用), 返回两次(在父进程中返回子进程的进程ID, 在子进程中返回0)*/
int
main(void)
{
	char	buf[MAXLINE];	/*MAXLINE 在 apue.h 中声明，常量*/
	pid_t	pid;
	int		status;

	printf("%% ");	/*打印提示符 (打印 % 需要 %% )*/

	/*fgets读入一行(默认以换行符结束), 每一行命令会产生一个子进程用于执行，
  当输入文件结束符(Ctrl+D)时，fgets返回null，这时循环终止*/
	while (fgets(buf, MAXLINE, stdin) != NULL) {
		/*去掉换行符，execlp函数需要的参数以null而不是换行符结束*/
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0;

		/*创建子进程进程执行读入的命令*/
		if ((pid = fork()) < 0) {
			err_sys("fork error");
		}
		/*对于子进程, fork返回的pid=0(父进程fork返回的pid>0)*/
		else if (pid == 0) {		/*开始执行子进程*/
			/*调用execlp以执行从标准输入读入的命令*/
			execlp(buf, buf, (char *)0);
			err_ret("couldn't execute: %s", buf);
			/*子进程退出*/
			exit(127);
		}
		/*子进程执行后，父进程,等待子进程终止*/
		/*pid为子进程id, status为子进程终止状态(用于判断其是如何终止的)*/

		if ((pid = waitpid(pid, &status, 0)) < 0)
			err_sys("waitpid error");

		printf("%% ");
	}
	exit(0);
}
