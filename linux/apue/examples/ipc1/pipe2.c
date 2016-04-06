/* 这个命令等价与在shell中运行 cat FILE | more */
#include "apue.h"
#include <sys/wait.h>

#define	DEF_PAGER	"/bin/more"		/* default pager program */

int
main(int argc, char *argv[])
{
	int		n;
	int		fd[2];
	pid_t	pid;
	char	*pager, *argv0;
	char	line[MAXLINE];
	FILE	*fp;

	if (argc != 2)
		err_quit("usage: a.out <pathname>");
	/* fp指向argv[1]指定的文件 */
	if ((fp = fopen(argv[1], "r")) == NULL)
		err_sys("can't open %s", argv[1]);
	if (pipe(fd) < 0)
		err_sys("pipe error");

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid > 0) {								/* 父进程 */
		close(fd[0]);		/* 关闭管道读端 */

		/* 父进程将fp(argv[1])写入管道 */
		while (fgets(line, MAXLINE, fp) != NULL) {
			n = strlen(line);
			if (write(fd[1], line, n) != n)
				err_sys("write error to pipe");
		}
		/* 判断是否出错 */
		if (ferror(fp))
			err_sys("fgets error");

		/* 关闭管道写端 */
		close(fd[1]);	/* 写完后，关闭管道写端 */

		/* 等待子进程执行完 */
		if (waitpid(pid, NULL, 0) < 0)
			err_sys("waitpid error");
		exit(0);
	} else {										/* 子进程 */
		close(fd[1]);	/* 关闭管道写端 */
		/* dup2(fd[0], STDIN_FILENO)会关闭标准输入，然后将STDIN_FILENO当做fp[0]的副本，这样做管道读端就成了标准输入*/
		if (fd[0] != STDIN_FILENO) {
			if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
				err_sys("dup2 error to stdin");
			close(fd[0]);	/* 关闭fp[0],这里不再需要了 */
		}

		/* get arguments for execl() */
		if ((pager = getenv("PAGER")) == NULL)
			pager = DEF_PAGER;
		/* 截取最后一个 '/' 之后的子串 */
		if ((argv0 = strrchr(pager, '/')) != NULL)
			argv0++;		/* step past rightmost slash */
		else
			argv0 = pager;	/* no slash in pager */
		/* 执行more命令,内容从管道读入 */
		if (execl(pager, argv0, (char *)0) < 0)
			err_sys("execl error for %s", pager);
	}
	exit(0);
}
