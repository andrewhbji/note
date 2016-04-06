#include "apue.h"
#include <errno.h>

/*strerror, 将errnum映射为一个出错信息字符串, 并且返回此字符串的指针*/
/*perror, 基于errno的当前值, 在标准出错上产生一条出错信息, 然后返回*/
/*EACCES和ENOENT在errno.h声明*/
int
main(int argc, char *argv[])
{
	fprintf(stderr, "EACCES: %s\n", strerror(EACCES));
	errno = EACCES;
	/*将程序名作为参数传递给perror，这时UNIX的惯例*/
	perror(argv[0]);
	exit(0);
}
