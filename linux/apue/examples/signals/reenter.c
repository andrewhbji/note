#include "apue.h"
#include <pwd.h>

static void
my_alarm(int signo)
{
	struct passwd	*rootptr;

	printf("in signal handler\n");
	/*getpwnam是不可重入函数，这里执行结果不可预期*/
	if ((rootptr = getpwnam("root")) == NULL)
			err_sys("getpwnam(root) error");
	alarm(1);
}

int
main(void)
{
	struct passwd	*ptr;

	signal(SIGALRM, my_alarm);
	alarm(1);
	for ( ; ; ) {
		if ((ptr = getpwnam("andrea")) == NULL)
			err_sys("getpwnam error");
		if (strcmp(ptr->pw_name, "andrea") != 0)
			printf("return value corrupted!, pw_name = %s\n",
					ptr->pw_name);
	}
}
