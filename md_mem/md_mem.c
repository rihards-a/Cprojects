/*
md_mem:

Finds the maximum amount of memory that can be reserved using
malloc(), mmap(), and sbrk(), one megabyte at a time.

We dont write to the allocated memory - we are measuring the
virtual address space limit, not how much physical RAM is present.

WHY EACH TEST RUNS IN A SEPARATE CHILD PROCESS:

	1. malloc() uses bookkeeping structures that consume real RAM.
	After millions of calls, the Linux system sends SIGKILL
	before malloc() ever returns NULL, killing the whole program.
	Because it's writing it's own headers to the reserved space.
	To get the amount of memory reserved back, shared memory is used.
 
	2. mmap() and sbrk() share the same virtual address space.
	If mmap() runs first and fills the address space, sbrk() will
	fail immediately (no contiguous space left for the heap) and
	report 0 MB.

Running each test in its own child process gives it a fresh,
empty address space, so the tests cannot interfere with each other.

The result is passed back via a MAP_SHARED region - a page of
memory that both parent and child map to the same physical address,
so writes in the child are visible to the parent even if the child
is killed before it can exit().
*/

#include <stdio.h>
#include <stdlib.h>      /* malloc */
#include <sys/mman.h>    /* mmap */
#include <sys/wait.h>    /* waitpid */
#include <unistd.h>      /* fork, sbrk */

#define MB (1024 * 1024)

/*
One shared page used to pass a count from child back to parent.
Initialized before fork() so both processes map the same page.
*/
long *g_count; /* usually would make this volatile, but since waitpid is being used it's fine */

void init_shared(void)
{
	/* PROT_READ/WRITE for those rights; MAP_SHARED to work between processes 
	; MAP_ANONYMOUS to ignore fd / not be bakced by a file and also be filled with 0s */
    void *p = mmap(NULL, sizeof(long), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); exit(1); }
    g_count = (long *)p;
}

/*
Forks fn() into a child, waits for it, returns the count.
*/
long run_in_child(void (*fn)(void)) /* (*fn) makes fn a function pointer, not a void pointer; the voids specify no return value and no arguments */
{
    *g_count = 0;
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

    return (long)*g_count;
}

/* ------------------------------------------------------------------ */
void child_malloc(void)
{
    while (malloc(MB) != NULL) {
        (*g_count)++;
	}
}

void child_mmap(void)
{
    while (mmap(NULL, MB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) != MAP_FAILED) {
        (*g_count)++;
	}
}

void child_sbrk(void)
{
    while (sbrk((intptr_t)MB) != (void *)-1) {
        (*g_count)++;
	}
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("Maximum reservable memory (1 MB at a time, separate address spaces for each function):\n\n");

    init_shared();

    printf("malloc : "); fflush(stdout); /* fflush to prevent the child from double printing the text after inheriting the stdio buffer */
    printf("%ld MB\n", run_in_child(child_malloc));

    printf("mmap   : "); fflush(stdout);
    printf("%ld MB\n", run_in_child(child_mmap));

    printf("sbrk   : "); fflush(stdout);
    printf("%ld MB\n", run_in_child(child_sbrk));

    return 0;
}
