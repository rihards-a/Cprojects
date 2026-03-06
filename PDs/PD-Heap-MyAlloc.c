/*
PD_Heap - myalloc nextfit implementation
*/

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define BUFFER_SIZE 4096

static unsigned char mybuffer[BUFFER_SIZE];

/* Every block (free or used) starts with this header. */
typedef struct Header {
    size_t size; /* number of usable payload bytes after this header */
    int free;
} Header;

#define HEADER_SIZE sizeof(Header) /* 16 bytes */

static int ready = 0;
static Header* last_pos = NULL; /* for next fit */

/* Return a pointer to the next block's header, or NULL if at the buffer end. */
static Header* next_hdr(Header* h)
{
    /*  cast to (unsigned char*) to increment the Header address by 1 byte per size when using +,
        instead of stepping by the size of the Header struct which is 16 bytes. And unsigned char
        is used instead of char, because char may have issues with some large values due to sign  */
    unsigned char* p = (unsigned char*)h + HEADER_SIZE + h->size;
    if (p >= mybuffer + BUFFER_SIZE) {
        return NULL;
    }
    return (Header*)p;
}

/*
Walk the whole buffer and merge neighbouring free blocks into one.
Example: [free 32][free 64][used 16]  becomes  [free 96][used 16]
*/
static void coalesce(void)
{
    Header* h = (Header*)mybuffer;
    while (h) {
        if (h->free) {
            Header* n;
            /* absorb every free block that comes right after this one */
            while ((n = next_hdr(h)) && n->free) {
                h->size += HEADER_SIZE + n->size; /* grow h to cover n's header and size */
            }
        }
        h = next_hdr(h);
    }
}

/* Set up the buffer as one big free block. Called automatically on first use. */
static void init(void)
{
    Header* h = (Header*)mybuffer;
    h->size = BUFFER_SIZE - HEADER_SIZE;
    h->free = 1;
    last_pos = h;
    ready = 1;
}

/*
-------------------------------------------------------------------------------
end of inaccessible static functions
-------------------------------------------------------------------------------
start of myalloc and myfree
-------------------------------------------------------------------------------
*/

/*
Allocate 'size' bytes and return a pointer to the usable memory.
Returns NULL if size is 0 or there is not enough space.

Next Fit: start searching from last_pos, wrap around once if needed,
give up only after a full lap with no suitable block found.
*/
void* myalloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    if (!ready) {
        init();
    }

    Header* start = last_pos;
    Header* h = start;
    int wrapped = 0;

    while (1) {
        if (h->free && h->size >= size) {
            /*  split the block if the leftover is large enough to be useful later. 
                8 is the minimal amount of space that can be reserved ( because of padding) */
            if (h->size >= size + HEADER_SIZE + 8) {
                Header* rest = (Header*)((unsigned char*)h + HEADER_SIZE + size);
                rest->size = h->size - size - HEADER_SIZE;
                rest->free = 1;
                h->size = size;
            }
            h->free = 0;
            last_pos = h; /* remember this spot for the next call */
            return (unsigned char*)h + HEADER_SIZE; /* return pointer past the header */
        }

        Header* next = next_hdr(h);
        if (!next) {
            if (wrapped) {
                return NULL; /* already looped once, nothing fits anywhere */
            }
            wrapped = 1;
            h = (Header*)mybuffer; /* wrap around to the start */
        } else {
            h = next;
        }

        if (h == start && wrapped) {
            return NULL; /* back where we started, full lap completed */
        }
    }
}

/*
Release a previously allocated block.
Returns 0 on success, otherwise -1.
*/
int myfree(void* ptr)
{
    if (!ptr) {
        return -1;
    }

    /* cast for 1 byte arithmetic operations */
    unsigned char* p = (unsigned char*)ptr;

    /* make sure the pointer is actually inside our buffer. */
    if (p < mybuffer + HEADER_SIZE || p >= mybuffer + BUFFER_SIZE) {
        return -1;
    }

    Header* h = (Header*)(p - HEADER_SIZE); /* step back to find the header */

    if (h->free) {
        return -1; /* already free, double free detected */
    }

    h->free = 1;
    coalesce(); /* merge with adjacent free blocks straight away */
    return 0;
}

/*
-------------------------------------------------------------------------------
end of main functions
-------------------------------------------------------------------------------
start of testing helper functions
-------------------------------------------------------------------------------
*/

/* visualize the buffer and it's segments */
void print_map(void)
{
    printf("  %-8s %-8s %s\n", "offset", "size", "status");
    for (Header* h = (Header*)mybuffer; h; h = next_hdr(h)) {
        printf("  %-8d %-8zu %s\n",
               (int)((unsigned char*)h - mybuffer),
               h->size,
               h->free ? "free" : "USED");
    }
    printf("\n");
}

#define CHECK(cond, msg) printf("  %s %s\n", (cond) ? "[PASS]" : "[FAIL]", msg)
#define RESET() do { ready = 0; last_pos = NULL; } while (0)

/* Test 1: edge cases that should not crash */
static void test_edges(void)
{
    printf("TEST 1 - edge cases\n");
    RESET();

    CHECK(myalloc(0) == NULL, "zero-size returns NULL");
    CHECK(myalloc(BUFFER_SIZE) == NULL, "too-large returns NULL");

    void* p = myalloc(1);
    CHECK(p != NULL, "1-byte alloc succeeds");
    *(unsigned char*)p = 7;
    CHECK(*(unsigned char*)p == 7, "1-byte write/read OK");
    myfree(p);

    int x;
    CHECK(myfree(&x) == -1, "out-of-buffer pointer returns -1");
}

/*
Test 2: Next Fit wrap-around

Fill the buffer completely, then free the first 2 blocks.
last_pos is still near the end of the buffer.
The next allocation must scan forward, find nothing, wrap to the start,
and land in the freed first slot.
*/
static void test_wraparound(void)
{
    printf("\nTEST 2 - Next Fit wrap-around\n");
    RESET();

    void* ptrs[64];
    int n = 0;
    while (n < 63) {
        void* p = myalloc(48); /* fill memory leaving a spot for jsut 1 more 48 byte reservation */
        ptrs[n++] = p;
    }

    myfree(ptrs[0]); /* open a slot at the very start; last_pos stays near the end */
    myfree(ptrs[1]); /* we can't insert something larger than 48 bytes at the end, we need to loop around */

    void* d = myalloc(80); /* scan forward from last_pos, find nothing, wrap around */
    CHECK(d != NULL, "wrap-around allocation succeeds");
    CHECK(d < (void*)(mybuffer + 100), "d landed near the start (wrapped)");

    print_map(); /* we can see that 80 bytes have been reserved and that there is free space at the end and between */
}

/*
Test 3: Timing

Randomly allocate and free randomly sized memory in the buffer over 1 million iterations,
using a linear congruential generator with glibc's numbers, effectively reimplementing
rand() for speed, so the benchmark is as accurate as possible.
*/
static void test_timing(void)
{
    printf("\nTEST 3 - timing\n");
    RESET();

    int i;
    const int p_c = 64;
    void* p[64] = {0}; /* can't use p_c because the compiler sees it as variable sized */
    uint32_t r = 1;
    const int ITERS = 1e6;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int _ = 0; _ < ITERS; _++) {
        /*  the same glibc LCG numbers used in rand(),
            trying not to disturb the benchmark by 
            writing a custom implementation here with less overhead */
        r = r * 1103515245 + 12345;

        /*  >> 16 is the same as dividing by 65536 = 2^16, apparently for better randomness it's better to use high bits
            this is based on: https://pubs.opengroup.org/onlinepubs/009695399/functions/rand.html  
            isntead of using `mod p_c` make sure p_c is a power of 2 and use the AND operation instead for speed */
        i = (r >> 16) & (p_c-1);

        /*  50/50 - either we try to allocate a random pointer or to free one 
            it depends on whether the lowest bit is on or off, then we use the random i index 
            and finally we allocate a random value between 1 and 64 */
        if ((r & 1)) {
            if (!p[i]) {
                p[i] = myalloc((r & 63)+1);
            }
        } else if (p[i]) {
            myfree(p[i]);
            p[i] = NULL;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Average per malloc/free: %.3f ns\n", (elapsed * 1e9) / ITERS);
}

int main(void)
{
    printf("=== Custom allocator (Next Fit), buffer = %d bytes ===\n\n", BUFFER_SIZE);
    test_edges();
    test_wraparound();
    test_timing();
    return 0;
}
