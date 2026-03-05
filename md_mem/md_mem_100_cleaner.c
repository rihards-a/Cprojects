/*
md_mem copy with reservation size limits and time logging using time.h
*/

#include <stdio.h>
#include <time.h>		 /* timespec */
#include <stdlib.h>      /* malloc */
#include <sys/mman.h>    /* mmap */
#include <sys/wait.h>    /* waitpid */
#include <unistd.h>      /* fork, sbrk */

#define MB (1024 * 1024)
#define LIMIT_MB (100)

/* 
Macro for running the allocations - they follow the same blueprint,
also using a macro doesn't introduce any overhead for benchmarking.
*/
#define CHILD_ALLOC(expr, failmsg)                  \
do {                                                \
    struct timespec t0, t1;                         \
    clock_gettime(CLOCK_MONOTONIC, &t0);            \
    for (size_t i = 0; i < LIMIT_MB; ++i) {         \
        if (expr) { printf("\t !%s failed to reserve 100MBs! \t", failmsg); break; } \
    }                                               \
    clock_gettime(CLOCK_MONOTONIC, &t1);            \
    *g_time_ns = ts_to_ns(&t1) - ts_to_ns(&t0);     \
} while(0) /* do {} while (0), is one way to allow us to define a macro like this */


/*
One shared page used to pass a count from child back to parent.
Initialized before fork() so both processes map the same page.
*/
int64_t *g_time_ns; /* usually would make this volatile, but since waitpid is used it's fine */

void init_shared(void)
{
    void *p = mmap(NULL, sizeof(int64_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); exit(1); }
    g_time_ns = (int64_t *)p;
}

int64_t ts_to_ns(const struct timespec *t)
{
    /* 
	timespec stores seconds and nanoseconds separately, to get ns returned, have to multiply seconds by a billion;
	LL to prevent overflows on 32-bit systems, because long long is guaranteed to be at least 64bits which is necessary    
    */
    return (int64_t)t->tv_sec * 1000000000LL + t->tv_nsec;
}

/*
Forks fn() into a child, waits for it, returns the count.
*/
void run_in_child(void (*fn)(void))  /* (*fn) makes fn a function pointer, not a void pointer; the voids specify no return value and no arguments */
{
    *g_time_ns = 0;
    fflush(stdout);

    pid_t pid = fork();              /* duplicate this process */
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {                  /* child: run the test and exit */
        fn();
        exit(0);
    }

    /* parent: wait for the child to finish or be killed */
    int status; /* status allows for checking how the child exited, but it's not really necessary */
    waitpid(pid, &status, 0);

    return;
}

/* ------------------------------------------------------------------ */
void child_malloc(void)
{
    CHILD_ALLOC((malloc(MB) == NULL), "Malloc");
}

void child_mmap(void)
{
    CHILD_ALLOC((mmap(NULL, MB, PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED), "Mmap");
}

void child_sbrk(void)
{
    CHILD_ALLOC((sbrk((intptr_t)MB) == (void *)-1), "Sbrk");
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("Time to reserve 100 MB (1 MB at a time):\n\n");

    init_shared();

    printf("malloc : "); fflush(stdout); /* fflush to prevent the child from double printing the text after inheriting the stdio buffer */
    run_in_child(child_malloc);
    printf("%.6f s\n", *g_time_ns / 1e9); /* float with 6 digit precision */

    printf("mmap   : "); fflush(stdout);
    run_in_child(child_mmap);
    printf("%.6f s\n", *g_time_ns / 1e9);

    printf("sbrk   : "); fflush(stdout);
    run_in_child(child_sbrk);
    printf("%.6f s\n", *g_time_ns / 1e9);
    
    return 0;
}
