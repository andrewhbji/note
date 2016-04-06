#include "apue.h"
#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>

void
daemonize(const char *cmd)
{
	int					i, fd0, fd1, fd2;
	pid_t				pid;
	struct rlimit		rl;
	struct sigaction	sa;

	/*
	 * Clear file creation mask.
	 * 重置文件权限掩码
	 */
	umask(0);

	/*
	 * Get maximum number of file descriptors.
	 * 获得最高的文件描述符
	 */
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		err_quit("%s: can't get file limit", cmd);

	/*
	 * Become a session leader to lose controlling TTY.
	 * 调用 fork 然后 exit 父进程，子进程调用 setid 创建一个会话
	 * 这样子进程就成了新会话的首进程，同时创建一个进程组并成为组长，且子进程没有控制终端
	 */
	if ((pid = fork()) < 0)
		err_quit("%s: can't fork", cmd);
	else if (pid != 0) /* parent */
		exit(0);
	setsid();

	/*
	 * Ensure future opens won't allocate controlling TTYs.
	 * 忽略掉控制终端发出的 SIGHUP 信号
	 */
	/* 将信号处理函数设置为忽略 */
	sa.sa_handler = SIG_IGN;
	/* 清空信号掩码 */
	sigemptyset(&sa.sa_mask);
	/* 关闭一切信号处理操作 */
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		err_quit("%s: can't ignore SIGHUP", cmd);
	/* 在基于System V的系统中，有人建议再fork一次，然后终止父进程，继续使用子进程中的守护进程。这样就保证了该守护进程不是会话首进程，按照 System V的规则，可以防止其获取控制终端 */
	if ((pid = fork()) < 0)
		err_quit("%s: can't fork", cmd);
	else if (pid != 0) /* parent */
		exit(0);

	/*
	 * Change the current working directory to the root so
	 * we won't prevent file systems from being unmounted.
	 * 将当前工作目录切换到 /,以避免挂载失败
	 */
	if (chdir("/") < 0)
		err_quit("%s: can't change directory to /", cmd);

	/*
	 * Close all open file descriptors.
	 */
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for (i = 0; i < rl.rlim_max; i++)
		close(i);

	/*
	 * Attach file descriptors 0, 1, and 2 to /dev/null.
	 */
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	/*
	 * Initialize the log file.
	 */
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d",
		  fd0, fd1, fd2);
		exit(1);
	}
}
