#include "apue.h"

int		globvar = 6;		/* external variable in initialized data */

int
main(void)
{
	int		var;		/* automatic variable on the stack */
	pid_t	pid;

	var = 88;
	printf("before vfork\n");	/* we don't flush stdio */
	if ((pid = vfork()) < 0) {
		err_sys("vfork error");
	} else if (pid == 0) {		/* child */
		globvar++;				/* 子进程修改父进程的变量 */
		var++;
		_exit(0);				/* 子进程结束 */
	}

	/* 父进程执行，发现变量已经改变 */
	printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar,
	  var);
	exit(0);
}
