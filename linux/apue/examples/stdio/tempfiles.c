#include "apue.h"

int
main(void)
{
	char	name[L_tmpnam], line[MAXLINE];
	FILE	*fp;

	printf("%s\n", tmpnam(NULL));		/* 取出静态区的临时文件名 */

	tmpnam(name);						/* 使用name保存临时文件名 */
	printf("%s\n", name);

	if ((fp = tmpfile()) == NULL)		/* 创建临时文件 */
		err_sys("tmpfile error");
	fputs("one line of output\n", fp);	/* 写入一行 */
	rewind(fp);							/* 初始化偏移量 */
	if (fgets(line, sizeof(line), fp) == NULL) /*读取一行到line*/
		err_sys("fgets error");
	fputs(line, stdout);				/* 将line输出到stdout */

	exit(0);
}
