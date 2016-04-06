#include "apue.h"

int		globvar = 6;		/* external variable in initialized data */
char	buf[] = "a write to stdout\n";

int
main(void)
{
	int		var;		/* automatic variable on the stack */
	pid_t	pid;

	var = 88;
	if (write(STDOUT_FILENO, buf, sizeof(buf)-1) != sizeof(buf)-1) /* sizeof运算符会包含NULL字节 */
		err_sys("write error");
	err_time();
	printf("before fork\n");	/* we don't flush stdout */

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid == 0) {		/* child */
		globvar++;				/* modify variables */
		var++;
	} else {
		sleep(2);				/* parent */
	}
	/* 子进程先打印，父进程休息两秒后打印 */
	err_time();
	printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar,
	  var);
	exit(0);
}
