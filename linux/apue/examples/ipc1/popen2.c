#include "apue.h"
#include <sys/wait.h>

#define	PAGER	"${PAGER:-more}" /* environment variable, or default */

int
main(int argc, char *argv[])
{
	char	line[MAXLINE];
	FILE	*fpin, *fpout;

	if (argc != 2)
		err_quit("usage: a.out <pathname>");
	/* 打开命令行参数指定的文件 */
	if ((fpin = fopen(argv[1], "r")) == NULL)
		err_sys("can't open %s", argv[1]);
	/* 创建写管道 */
	if ((fpout = popen(PAGER, "w")) == NULL)
		err_sys("popen error");

	/* 将文件内容写到管道的stdin */
	while (fgets(line, MAXLINE, fpin) != NULL) {
		if (fputs(line, fpout) == EOF)
			err_sys("fputs error to pipe");
	}
	if (ferror(fpin))
		err_sys("fgets error");
	if (pclose(fpout) == -1)
		err_sys("pclose error");

	exit(0);
}
