#include "slock.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* 分配信号量 */
struct slock *
s_alloc()
{
	struct slock *sp;
	static int cnt;

	if ((sp = malloc(sizeof(struct slock))) == NULL)
		return(NULL);
	/* 循环生成信号量，直到 sem_open 返回SEM_FAILED或信号量已经存在 */
	do {
		/* 生成信号量name */
		snprintf(sp->name, sizeof(sp->name), "/%ld.%d", (long)getpid(),
		  cnt++);
		/* 初始化信号量;权限为所有者RWX,参考第四章 file_and_dir;初始值为1 */
		sp->semp = sem_open(sp->name, O_CREAT|O_EXCL, S_IRWXU, 1);
	} while ((sp->semp == SEM_FAILED) && (errno == EEXIST));
	/* 如果 sem_open 返回 SEM_FAILED，释放内存，返回 */
	if (sp->semp == SEM_FAILED) {
		free(sp);
		return(NULL);
	}
	/* 先请求删除信号量，因为之前open的还没有调用close，所以暂时不删除*/
	sem_unlink(sp->name);
	return(sp);
}

/* 释放信号量 */
void
s_free(struct slock *sp)
{
	/* 关闭打开的信号量 */
	sem_close(sp->semp);
	free(sp);
}

/* 锁定 */
int
s_lock(struct slock *sp)
{
	return(sem_wait(sp->semp));
}

/* 尝试锁定 */
int
s_trylock(struct slock *sp)
{
	return(sem_trywait(sp->semp));
}

/* 解锁 */
int
s_unlock(struct slock *sp)
{
	return(sem_post(sp->semp));
}
