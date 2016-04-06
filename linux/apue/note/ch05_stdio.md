[TOC]
# 第5章 标准I/O库
标准I/O库最大的好处就是不需要再和底层内核调用打交道，便于跨平台使用。

## 流和FILE对象
使用stdio库打开或创建文件时，就将一个流和文件关联，流处理文件中的字符
>备注：字符可以分为ASCII字符(单字节)和国际字符集(多字节)

### fwide
设置流的宽度(单字节还是多字节)
```c
#include <stdio.h>
#include <wchar.h>
int fwide(FILE *fp, int mode);
返回值：流被设置为多字节则返回正值；流被设置为单字节则返回负值；流的宽度尚未设置则返回0
```
- mode 小于 0，则流将被设置为单字节；mode 大于 0，流将被设置为多字节；mode 等于 0，则不改变流的宽度
- fwide不会改变已经设置过宽度的流
- 无论是否改变，返回值都会存在，用于确定流的方向

## 标准输入、标准输出 和 标准错误
stdio.h 预定义文件指针stdin、stdout、stderr 可以引用文件描述符STDIN_FILENO、STDOUT_FILENO、STDERR_FILENO

## 缓冲
stdio提供三种缓冲类型
- 全缓冲：也叫块缓冲，标准IO函数会先使用malloc来获得固定大小的缓冲区，每当缓冲区被填满后就会进行实际的磁盘写入，开发者也可以手动调用fflush函数来强制将缓冲区写入磁盘
- 行缓冲：输入输出时遇到换行符，自动进行I/O写入；或者stdio的缓冲区被填满时自动进行I/O写入
- 不带缓冲：就如同Unix系统提供的write函数一样，stdio不对字符进行缓冲存储

>备注：
>1. stderr通常是不带缓冲的，因为错误信息通常需要得到立即的输出处理
>2. 指向终端的流时行缓冲，其他则是全缓冲

### setbuf setvbuf
更改缓冲类型
```c
#include <stdio.h>
void setbuf(FILE *restrict stream, char *restrict buf);

int setvbuf(FILE *restrict stream, char *restrict buf, int type, size_t size);
返回值：成功返回0，失败返回非0值
```
- setbuf函数用于打开关闭缓冲机制，将一个长度为BUFSIZ的缓冲区传入参数，就会打开缓冲区，而传入null则会关闭缓冲区。
- setvbuf函数功能十分强大，可以使用 type 参数精确的说明缓冲类型
```c
_IONBF /*无缓冲*/
_IOLBF /*行缓冲*/
_IOFBF /*全缓冲*/
```
- 一个不带缓冲的流可以忽略type和size参数
- 当一个缓冲传入的buf是null时，系统会自动分配缓冲区
- setbuf 基本等价于 就等同于setvbuf(stream, buf, buf ? \_IOFBF : \_IONBF, BUFSIZ)，除了没有返回值

### fflush
强制将缓冲区写入硬盘
```c
#include <stido.h>
int fflush(FILE *stream);
```

## fopen freopen fdopen fclose
打开、关闭流
```c
#include <stido.h>
FILE *fopen(const char *restrict filename, const char *restrict mode);
FILE *freopen(const char *restrict filename, const char *restrict mode, FILE *restrict stream);
FILE *fdopen(int fildes, const char *mode);
返回值：成功返回file指针，失败返回null

int fclose(FILE *stream);
返回值：成功返回0，失败返回EOF
```
- fopen 打开指定的文件
- freopen 在指定的流上打开指定的文件，如果该流已经打开，则先关闭该流。如果流的宽度已经指定，则设置为未指定
- 通过stdio库创建的文件只有0666权限，但是会被umask掩码限制

### fdopen 函数
fdopen 将文件描述符和流关联
- 文件描述符必须存在
- 流的模式必须兼容文件描述符
- 当用fclose关闭时，文件描述符也被关闭

### mode 参数
| mode 值     | 说明     | 对应 open函数的flag参数     |
|:--|:--|:--|
| r/rb       | 读打开       | O_RDONLY      |
| w/wb       | 写打开       | O_WRONLY or O_CREAT or O_TRUNC      |
| a/ab       | 追加       | O_WRONLY or O_CREAT or O_APPEND      |
| r+/r+b/rb+       | 读写打开       | O_RDWR      |
| w+/w+b/wb+       | 读写打开       | O_RDWR or O_CREAT or O_TRUNC      |
| a+/a+b/ab+       | 文件尾读写打开       | O_RDWR or O_CREAT or O_APPEND      |

### fclose
很简单，就是将缓冲区内容写入磁盘并关闭文件，如果缓冲区是自动分配则会自动回收缓冲区。

## 一次对一个字符I/O

### getc fgetc getchar
```c
#include <stido.h>
int getc(FILE *stream);
int fgetc(FILE *stream);
int getchar(void);
返回值：成功返回下一个字符，失败或者到达流结尾返回EOF
```
- getc 是宏定义，从流中获取下一个输入的字符
- fgetc 和getc差不多，只不过fgetc是函数
- getchar() 等价与 getc(stdin)

### ferror feof clearerr
ferror和feof用于区分error和EOF，clearerr用于清除这两个标志
```c
#include <stdio.h>
int ferror(FILE *stream);
int feof(FILE *stream);
返回值：条件为真返回1，假返回0
void clearerr(FILE *stream);
```

### ungetc
将已经读取的字符反压回流
```c
#include <stdio.h>
int ungetc(int c, FILE *stream);
返回值：成功返回c；出错返回EOF
```

### putc fputc putchar
基本上同input函数
```c
int putc(int c, FILE *stream);
int fputc(int c, FILE *stream);
int putchar(int c);
返回值：成功返回c；出错返回EOF
```

## 一次对一行I/O

### fgets gets
- fgets 函数从指定的流中读取不超过size指定的字符串，并保存到str中，直到读到换行符、EOF或者error，并且str会以null字节结尾
- gets 函数等价于指定流为stdin，并且无限size的fgets函数。使用者必须保证输入足够短，否则可能导致缓冲区溢出
```c
#include <stdio.h>
char *fgets(char * restrict str, int size, FILE * restrict stream);
char *gets(char *str);
返回值：成功返回buf，若出错或已到达字符串结尾则返回null
```

### fputs puts
- puts 将以null符终止的字符串输出到stdout，输出时会将null符替换为换行符
- fputs 将以null符终止的的字符串输出到指定的流，不会输出最后那个null符，但也不会将null符替换为换行符。一般需要在null符增加一个换行符
```c
#include <stdio.h>
int fputs(const char *restrict s, FILE *restrict stream);
int puts(const char *s);
返回值：成功返回非负值，出错返回EOF
```

## fread fwrite
fread和fwrite操作的是二进制对象(比如基本数据类型变量和结构体变量，单位为个)
```c
#include <stdio.h>
size_t fread(void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream);
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream);
返回值：读写的对象数目，如果到达了底部或者出错，则返回实际写入的对象数和0
```
- 参数 size 指定读入/写入的空间大小，通常用sizeof获取
- 参数 nitems 指定读入/写入的对象数量
- fread 将从流中读入的内容保存在 prt 中
- fwrite 的 ptr 参数指定需要写入流的对象的地址
```c
#include <stdio.h>
float data[10];
if (fwrite(&data[2], sizeof(float), 4, fp) != 4) /*将数组的第2、3、4、5个元素写入到fp*/
  err_sys("fwrite error");

struct{
  short count;
  long total;
  char name[NAMESIZE];
}item；
if (fwrite(&item, sizeof(item), 1, fp) != 1) /*将item结构体变量写入到fp*/
  err_sys("fwrite error");
```

## 流定位

### ftell fseek
查询和设置流的偏移量，这两个函数太古老，最好少用
>这些函数单位是都是字节，其中whence和Unix系统的lseek函数是一样的
```c
#include <stdio.h>
long ftell(FILE *stream);
返回值：若成功返回当前偏移量，出错则返回-1L
int fseek(FILE *stream, long offset, int whence);
返回值：成功返回0，出错返回非0值
```

### ftello fseeko
除了单位不同，和ftell、fseek没有区别
```c
#include <stdio.h>
off_t ftello(FILE *stream);
返回值：若成功返回当前偏移量，出错则返回-1
int fseeko(FILE *stream, off_t offset, int whence);
返回值：成功返回0，出错返回非0值
```

### fgetpos fsetpos
- fgetpos 函数将指定流的偏移量记录在pos中
- fsetpos 函数设置指定流的偏移量设置为pos
- 这两个函数是被ISO C引入的，使用抽象文件位置记录位置，跨平台推荐使用
```c
#include <stdio.h>
int fgetpos(FILE *restrict stream, fpos_t *restrict pos);
int fsetpos(FILE *stream, const fpos_t *pos);
返回值：成功返回0，出错返回非0值
```

### rewind
重置流的偏移量
```c
#include <stdio.h>
void rewind(FILE *stream);
```

## 格式化I/O

### printf fprintf dprintf sprintf snprintf
- printf就是向标准输出写，fprintf是向指定流写，dprintf是向文件描述符写
- sprintf和snprintf都是是向字符串 str 写，但是snprintf加入了size参数确定大小，sprintf由于存在缓冲区溢出的隐患，所以也不建议使用了
```c
#include <stdio.h>
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int dprintf(int fd, const char *restrict format, ...);
返回值：成功返回输出的字符数；若输出出错返回负值
int sprintf(char *restrict str, const char *restrict format, ...);
返回值：成功返回存入数组的字符数；出错返回负值
int snprintf(char *restrict str, size_t size, const char *restrict format, ...);
返回值：若缓冲区足够大，返回存入数组的字符数；若编码出错，返回负值
```

### vprintf vfprintf vsprintf vsnprintf vdprintf
这些函数是printf函数族的变体,被放在<stdarg.h>文件中，只是将可变参数表改成了va_list
```c
#include <stdarg.h>
int vprintf(const char *restrict format, va_list ap);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);
int vdprintf(int fd, const char * restrict format, va_list ap);
返回值：成功返回输出的字符数；若输出出错返回负值
int vsprintf(char *restrict str, const char *restrict format, va_list ap);
返回值：成功返回存入数组的字符数；出错返回负值
int vsnprintf(char *restrict str, size_t size, const char *restrict format, va_list ap);
返回值：若缓冲区足够大，返回存入数组的字符数；若编码出错，返回负值
```

### scanf fscanf sscanf
分别从stdin、流 以及字符串输入格式化输入
```c
#include <stdio.h>
int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict stream, const char *restrict format, ...);
int sscanf(const char *restrict s, const char *restrict format, ...);
返回值：成功返回被赋值的变量个数；若输入出错或在任意转换前到达文件尾端，返回EOF
```
这些函数都根据 format 来分析输入的字符串

### vscanf vfcanv vsscanf
scanf 函数族的变体，同样用va_last替换了可变参数表
```c
#include <stdarg.h>
int vscanf(const char *restrict format, va_list arg);
int vfscanf(FILE *restrict stream, const char *restrict format, va_list arg);
int vsscanf(const char *restrict s, const char *restrict format, va_list arg);
返回值：成功返回被赋值的变量个数；若输入出错或在任意转换前到达文件尾端，返回EOF
```

## tmpnam tmpfile
- tmpnam函数产生一个有效路径名字符串，如果s参数为null，则所产生的路径存放在静态区，然后将指针返回，当继续调用时，将会重写整个静态区，如果s不是null，则将其存放在s中，s也作为函数值返回
>tmpnam 最多能调用TMP_MAX(在stdio.h中定义)次

- tmpfile函数创建一个临时文件，这个文件在close或者程序结束后自动删除
```c
#include <stdio.h>
char *tmpnam(char *s);
返回值：返回临时文件名
FILE *tmpfile(void);
返回值：成功返回文件指针，失败返回NULL
```
```c
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
```

## mkdtemp mkstemp
- mkdtemp 使用模板(固定部分+浮动部分，如dirXXXXXX，浮动部分)创建临时路径，路径权限为S_IRUSR | S_IWUSR | S_IXUSR，并会被umask限制权限
- mkstemp 使用模板创建创建临时，并自动打开该文件，文件权限为S_IRUSR | S_IWUSR；mkstemp生成的文件需要 unlink 后才能删除
```c
#include <stdlib.h>
char *mkdtemp(char *template);
返回值：成功返回路径名，失败返回NULL
int mkstemp(char *template);
返回值：成功返回文件描述符，失败返回-1
```
> 备注：多进程调用tmpnam时，可能会生成同名的临时文件名。所以一般使用tempfile 和 mkstemp代替 tmpnam

## 内存流
stdio库中操作内存流的方式和操作文件流基本一致

### fmemopen open_memstream open_wmemstream
stdio提供三个函数创建内存流
```c
#include <stdio.h>
FILE *fmemopen(void *restrict buf, size_t size, const char *restrict type);
FILE *open_memstream(char **bufp, size_t *sizep);

#include <wchar.h>
FILE *open_wmemstream(wchar_t **bufp, size_t *sizep);
返回值：成功返回stream指针，失败返回NULL
```
- fmemopen 允许调用者提供一个缓冲区buf，将流中的内容写入buf，设置缓冲区的大小为size。如果buf为空，fmemopen自动分配一个大小为size的buf，并且流关闭时，这个buf将释放；type 参数控制流的使用方式，基本上和fdopen 的 mode 参数一致
>备注：
>1. 因为以追加模式操作流是以判断NULL字节的位置进行的，内存流不适合操作二进制对象(NULL字节可能出现再二进制对象中间某位置)
>2. 对于 buf 为NULL的内存流，只读和只写时没有意义的，因为只读时，流为空，只读没有意义；只写时，虽可以写入，但是不可读，也没有意义。
>3. 调用 fclose，fflush，fseek，fseeko 或 fsetpos时，都会在流写入buf，需要注意NULL字符在buf中的位置，fseek fclose fflush 都会将NULL写入 buf的最后，fseek 会将 流连同NULL原样写入buf，导致buf截断

- open_memstream 面向的是单字节流，open_wmemstream 面向的是宽字节流
>备注：
>1. open_memstream 和 open_wmemstream 创建的流只能写
>2. 这两个函数不能指定自己的buf，但可以通过 bufp 和 sizep 访问 buf的地址
>3. 关闭流后需要自己释放buf
>4. buf 的大小根据 流的大小浮动变化
>5. buf 的地址和大小只在fclose、fflush、以及写入流后生效
>6. fclose 或者 fflush后，buf 可能会重新分配，那时需要对 buf 重新寻址
