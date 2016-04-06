#include "apue.h"

#define BSZ 48

int
main()
{
	FILE *fp;
	char buf[BSZ];

	memset(buf, 'a', BSZ-2); /*将buf置为46个a*/
	buf[BSZ-2] = '\0';
	buf[BSZ-1] = 'X';
	if ((fp = fmemopen(buf, BSZ, "w+")) == NULL)
		err_sys("fmemopen failed");
	printf("initial buffer contents: %s\n", buf); /*w+将buf截断为0(将先前写入的a清空)*/
	fprintf(fp, "hello, world");	/*将hello world写入流开始处,之后offset被修改为12*/
	printf("before flush: %s\n", buf);
	fflush(fp);
	printf("after fflush: %s\n", buf); /*将流写入buf*/
	printf("len of string in buf = %ld\n", (long)strlen(buf));

	memset(buf, 'b', BSZ-2); /*将buf重置为46个b*/
	buf[BSZ-2] = '\0';
	buf[BSZ-1] = 'X';
	fprintf(fp, "hello, world"); /*将hello world写入流的offset(12)位，之后offset被修改为24*/
	fseek(fp, 0, SEEK_SET); /*seek到流的起始位，同时将流写入到buf的12，buf从第24位设置为NULL*/
	printf("after  fseek: %s\n", buf);
	printf("len of string in buf = %ld\n", (long)strlen(buf));

	memset(buf, 'c', BSZ-2); /*将buf重置为46个c*/
	buf[BSZ-2] = '\0';
	buf[BSZ-1] = 'X';
	fprintf(fp, "hello, world"); /*将hello world写入流的offset(0),之后offset被修改为12*/
	fclose(fp); /*关闭流，同时将流写入buf的0到11位*/
	printf("after fclose: %s\n", buf);
	printf("len of string in buf = %ld\n", (long)strlen(buf));

	return(0);
}
