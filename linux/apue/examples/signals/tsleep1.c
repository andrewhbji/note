#include "apue.h"

static void		sig_int(int);
unsigned int	sleep1(unsigned int);

int
main(void)
{
    unsigned int	unslept;

    if (signal(SIGINT, sig_int) == SIG_ERR)
        err_sys("signal(SIGINT) error");
    unslept = sleep1(5);
    printf("sleep1 returned: %u\n", unslept);
    exit(0);
}

static void
sig_int(int signo)
{
    int				i, j;
    volatile int	k;

    /*
     * Tune these loops to run for more than 5 seconds
     * on whatever system this test program is run.
     */
    printf("\nsig_int starting\n");
    for (i = 0; i < 300000; i++)
        for (j = 0; j < 4000; j++)
            k += i * j;
    printf("sig_int finished\n");
}
