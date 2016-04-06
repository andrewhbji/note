#include "apue.h"
#include <sys/wait.h>

static void charatatime(char *);

int
main(void)
{
	pid_t	pid;
	int		status;

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid == 0) {
		charatatime("output from child\n");
		exit(0);
	} else {
		wait(&status);
		charatatime("output from parent\n");
	}
	exit(0);
}

static void
charatatime(char *str)
{
	char	*ptr;
	int		c;

	setbuf(stdout, NULL);			/* set unbuffered */
	for (ptr = str; (c = *ptr++) != 0; )
		putc(c, stdout);
}
