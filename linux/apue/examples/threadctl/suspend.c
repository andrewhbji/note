#include "apue.h"
#include <pthread.h>

int			quitflag;	/* set nonzero by thread */
sigset_t	mask;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitloc = PTHREAD_COND_INITIALIZER;

void *
thr_fn(void *arg)
{
	int err, signo;

	for (;;) {
		/* 线程启动后阻塞在这里，等待SIGINT SIGQUIT信号出现 */
		err = sigwait(&mask, &signo);
		if (err != 0)
			err_exit(err, "sigwait failed");
		switch (signo) {
		case SIGINT:
			/* 处理 SIGINT */
			printf("\ninterrupt\n");
			break;

		case SIGQUIT:
			/* 处理SIGQUIT,锁定并修改quitflag，然后向主线程发送条件成立信号 */
			pthread_mutex_lock(&lock);
			quitflag = 1;
			pthread_mutex_unlock(&lock);
			pthread_cond_signal(&waitloc);
			return(0);

		default:
			printf("unexpected signal %d\n", signo);
			exit(1);
		}
	}
}

int
main(void)
{
	int			err;
	sigset_t	oldmask;
	pthread_t	tid;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	/* 主线程阻塞 SIGINT SIGQUIT*/
	if ((err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask)) != 0)
		err_exit(err, "SIG_BLOCK error");

	err = pthread_create(&tid, NULL, thr_fn, 0);
	if (err != 0)
		err_exit(err, "can't create thread");

	/* 主线程条件阻塞 */
	pthread_mutex_lock(&lock);
	while (quitflag == 0)
		/* 当quitflag==1条件成立后，解除阻塞 */
		pthread_cond_wait(&waitloc, &lock);
	pthread_mutex_unlock(&lock);

	/* SIGQUIT has been caught and is now blocked; do whatever */
	quitflag = 0;

	/* 重置信号掩码，SIGQUIT解除阻塞 */
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
		err_sys("SIG_SETMASK error");
	exit(0);
}
