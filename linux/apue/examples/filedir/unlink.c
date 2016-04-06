#include "apue.h"
#include <fcntl.h>

int
main(void)
{
	if (open("bar", O_RDWR) < 0)
		err_sys("open error");
	if (unlink("bar") < 0)
		err_sys("unlink error");
	printf("file unlinked\n");
	sleep(15);
	printf("done\n");
	exit(0);
}
