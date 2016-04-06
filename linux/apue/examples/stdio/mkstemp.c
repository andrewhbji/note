#include "apue.h"
#include <errno.h>

void make_temp(char *template);

int
main()
{
	char	good_template[] = "/tmp/dirXXXXXX";	/* 正确的方式，""引用的内容已全部分配进栈内存中，可以被mkstemp修改 */
	char	*bad_template = "/tmp/dirXXXXXX";	/* 错误的方式，""引用的内容仍在只读区中，不能被mkstemp修改 */

	printf("trying to create first temp file...\n");
	make_temp(good_template);
	printf("trying to create second temp file...\n");
	make_temp(bad_template);
	exit(0);
}

void
make_temp(char *template)
{
	int			fd;
	struct stat	sbuf;

	if ((fd = mkstemp(template)) < 0)
		err_sys("can't create temp file");
	printf("temp name = %s\n", template);
	close(fd);
	if (stat(template, &sbuf) < 0) {
		if (errno == ENOENT)
			printf("file doesn't exist\n");
		else
			err_sys("stat failed");
	} else {
		printf("file exists\n");
		unlink(template);
	}
}
