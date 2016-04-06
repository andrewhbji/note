# 指定make根路径
ROOT=..
# 执行systype.sh获取判断系统平台
PLATFORM=$(shell $(ROOT)/systype.sh)
# 根据系统平台设置编译环境
include $(ROOT)/Make.defines.$(PLATFORM)
# 指定编译目标
PROGS =	getcputc hello ls1 mycat shell1 shell2 testerror uidgid

# 执行编译
all:	$(PROGS)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

# 清理
clean:
	rm -f $(PROGS) $(TEMPFILES) *.o
# 编译并设置lib目录
include $(ROOT)/Make.libapue.inc
