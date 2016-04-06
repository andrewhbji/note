#include "apue.h"

static void	sig_pipe(int);		/* our signal handler */

int
main(void)
{
	int		n, fd1[2], fd2[2];
	pid_t	pid;
	char	line[MAXLINE];

	/* 子进程被kill后，再向管道写入数据时，父进程处理SIGPIPE信号 */
	if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
		err_sys("signal error");

	/* 创建两个管道 fd1 和 fd2 */
	if (pipe(fd1) < 0 || pipe(fd2) < 0)
		err_sys("pipe error");

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid > 0) {							/* 父进程 */
		/* 关闭 fd1 的读端和 fd2 的写端*/
		close(fd1[0]);
		close(fd2[1]);

		/* 读取 stdin */
		while (fgets(line, MAXLINE, stdin) != NULL) {
			n = strlen(line);
			/* 将父进程的 stdin 通过 fd1 写入到子进程的 stdin  */
			if (write(fd1[1], line, n) != n){
				err_sys("write error to pipe");
			}
			/*
			else{
			 // write 成功则打印输出
				write(STDOUT_FILENO, "write fd1\n", 9);
			}*/
			/* 通过 fd2 读取子进程的 stdout  */
			if ((n = read(fd2[0], line, MAXLINE)) < 0){
				err_sys("read error from pipe");
			}
			/*
			else{
			 	//read 成功则打印输出
				write(STDOUT_FILENO, "read fd2\n", 8);
			}*/
			if (n == 0) {
				err_msg("child closed pipe");
				break;
			}
			line[n] = 0;	/* null terminate */
			/* 将从 fd2 读取的数据(子进程的stdout)写入父进程的 stdout */
			if (fputs(line, stdout) == EOF)
				err_sys("fputs error");
		}

		if (ferror(stdin))
			err_sys("fgets error on stdin");
		exit(0);
	} else {									/* 子进程 */
		/* 关闭 fd2 的读端和 fd1 的写端*/
		close(fd1[1]);
		close(fd2[0]);
		/* 子进程将fd1的读端当做stdin */
		if (fd1[0] != STDIN_FILENO) {
			if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO)
				err_sys("dup2 error to stdin");
			close(fd1[0]);
		}

		/* 子进程将fd2的写端当做stdout */
		if (fd2[1] != STDOUT_FILENO) {
			if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO)
				err_sys("dup2 error to stdout");
			close(fd2[1]);
		}
		/* 执行命令，从 fd1 读取数据，将结果写入 fd2 */
		if (execl("./add2", "add2", (char *)0) < 0)
			err_sys("execl error");
	}
	exit(0);
}

static void
sig_pipe(int signo)
{
	printf("SIGPIPE caught\n");
	exit(1);
}
