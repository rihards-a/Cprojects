#include <stdio.h>
#include <stdlib.h>

#define MAX_TIME 2000000000U
#define MAX_CLIENTS 200000U

typedef struct { unsigned int id, available_from, last_client_time; } Dresser;
typedef struct { unsigned int time, dresser_id, client_nr; } Result;

/* heapsort implementation for max points - see below */
static void heapsort_results(Result *a, unsigned int count);
static void heapify(Result *a, unsigned int count);
static void sift_down(Result *a, unsigned int root, unsigned int end);
static int  result_less(const Result *a, const Result *b);

/* earliest time >= t at which this dresser can begin serving a job of unit size needed */
static unsigned int next_available(const Dresser *d, unsigned int t, unsigned int needed) {
    unsigned int start = (d->available_from > t) ? d->available_from : t;
    unsigned int cycle_base = (start / 1000) * 1000;
    unsigned int t_in_cycle = start - cycle_base;
    unsigned int break_start = d->id * 100;

    /* currently inside break? skip to end of it and by the end 900 time units are available */
    if (break_start <= t_in_cycle && t_in_cycle < break_start + 100) {
        return start + (break_start + 100 - t_in_cycle);
    }
    /* is there enough time before the next break? */
    unsigned int until_break = (break_start - t_in_cycle + 1000) % 1000;
    if (until_break >= needed) {
        return start;
    }
    /* jump to right after the next break */
    unsigned int after_break = cycle_base + break_start + 100;
    if (after_break <= start) {
        /* edge case of X900 + X100 = X000 */
        after_break += 1000;
    }
    return after_break;
}

int main(void) {
    if (!freopen("hair.in", "r", stdin))   { perror("freopen hair.in");  return 1; }
    if (!freopen("hair.out", "w", stdout)) { perror("freopen hair.out"); return 1; }

    unsigned int dresser_count;
    if (scanf("%u", &dresser_count) != 1) { return 1; }

    Dresser *dressers = (Dresser *)malloc(dresser_count * sizeof(Dresser));
    if (!dressers) { return 1; }
    for (unsigned int i = 0; i < dresser_count; i++) {
        dressers[i].id = i + 1;
        dressers[i].available_from = 0;
        dressers[i].last_client_time = 0;
    }

    Result *results = (Result *)malloc(MAX_CLIENTS * sizeof(Result));
    if (!results) { free(dressers); return 1; }
    unsigned int result_count = 0;
    unsigned int current_time = 0;

    /* go through clients without storing them in memory, they're already ordered */
    unsigned int cl_time, cl_nr, cl_duration;
    while (scanf("%u", &cl_time) == 1 && cl_time != 0) {
        if (scanf("%u %u", &cl_nr, &cl_duration) != 2) { free(dressers); return 1; }

        unsigned int earliest           = (cl_time > current_time) ? cl_time : current_time;
        unsigned int best_start         = MAX_TIME + 1;
        unsigned int best_last_client   = MAX_TIME + 1;
        unsigned int best_id            = MAX_TIME + 1;
        unsigned int best_idx 			= 0;

        for (unsigned int i = 0; i < dresser_count; i++) {
            if (dressers[i].available_from > best_start) {
				/* can terminate since the dresser array is ordered by availability */
                break;
			}

            unsigned int t = next_available(&dressers[i], earliest, cl_duration);
            if (t < best_start
                || (t == best_start && dressers[i].last_client_time < best_last_client)
                || (t == best_start && dressers[i].last_client_time == best_last_client
                    && dressers[i].id < best_id)) {
                best_start       = t;
                best_last_client = dressers[i].last_client_time;
                best_id          = dressers[i].id;
				best_idx		 = i;
            }
        }

        dressers[best_idx].available_from   = best_start + cl_duration;
        dressers[best_idx].last_client_time = dressers[best_idx].available_from - 1;
        results[result_count].time       = dressers[best_idx].last_client_time;
        results[result_count].dresser_id = dressers[best_idx].id;
        results[result_count].client_nr  = cl_nr;
        result_count++;
        current_time = best_start;

        /* move the newly assigned dresser back in the array while the dresser behind it is available sooner */
        unsigned int end_idx = best_idx + 1;
        while (end_idx < dresser_count && dressers[end_idx].available_from < dressers[best_idx].available_from) {
            end_idx++;
        }
        if (end_idx > best_idx + 1) {
            Dresser tmp = dressers[best_idx];
            for (unsigned int i = best_idx; i < end_idx - 1; i++) {
                dressers[i] = dressers[i + 1];
            }
            dressers[end_idx - 1] = tmp;
        }
    }

	/* use heapsort instead - not too complex and works in-place with constant memory and O(nlog n)*/
    /* qsort(results, result_count, sizeof(Result), cmp_results); */
	heapsort_results(results, result_count);

    for (unsigned int i = 0; i < result_count; i++)
        printf("%u %u %u\n", results[i].time, results[i].dresser_id, results[i].client_nr);

    free(dressers);
    free(results);
    return 0;
}

static void heapsort_results(Result *a, unsigned int count) {
    heapify(a, count);
    unsigned int end = count;
	/* move the largest value - the first one to the very back
	   effectively sorting everything in ascending order */
    while (end-- > 1) {
        Result tmp = a[end]; a[end] = a[0]; a[0] = tmp;
        sift_down(a, 0, end);
    }
}

static void heapify(Result *a, unsigned int count) {
	/* is equivalent to iParent(count-1) + 1
	   because the last leaf is at count-1 and its parent
	   is at (count/2)-1 from child = 2*root+1 */
    unsigned int start = count / 2;
    while (start-- > 0) {
		/* effectively sift down each element starting 
		   from the back getting the largest value to the start 
		   while ensuring that all children are correctly ordered */
        sift_down(a, start, count);
	}
}

static void sift_down(Result *a, unsigned int root, unsigned int end) {
    while (2 * root + 1 < end) { /* while root has a child */
        unsigned int child = 2 * root + 1; /* property of binary trees in an array */
        if (child + 1 < end && result_less(&a[child], &a[child + 1])) {
            child++;
		}
        if (result_less(&a[root], &a[child])) {
            Result tmp = a[root]; a[root] = a[child]; a[child] = tmp;
            root = child; /* continue sifting down to children */ 
        } else {
            return;
        }
    }
}

/* returns ( a < b ) with Result property comparisons */
static int result_less(const Result *a, const Result *b) {
    if (a->time != b->time) {
        return a->time < b->time;
	}
    return a->dresser_id < b->dresser_id;
}

/* heapsort implementation wiki example and base:
procedure heapsort(a, count) is
    input: an unordered array a of length count
 
    (Build the heap in array a so that largest value is at the root)
    heapify(a, count)

    (The following loop maintains the invariants that a[0:end−1] is a heap, and
    every element a[end:count−1] beyond end is greater than everything before it,
    i.e. a[end:count−1] is in sorted order.)
    end ← count
    while end > 1 do
        (the heap size is reduced by one)
        end ← end − 1
        (a[0] is the root and largest value. The swap moves it in front of the sorted elements.)
        swap(a[end], a[0])
        (the swap ruined the heap property, so restore it)
        siftDown(a, 0, end)
		
(Put elements of 'a' in heap order, in-place)
procedure heapify(a, count) is
    (start is initialized to the first leaf node)
    (the last element in a 0-based array is at index count-1; find the parent of that element)
    start ← iParent(count-1) + 1
    
    while start > 0 do
        (go to the last non-heap node)
        start ← start − 1
        (sift down the node at index 'start' to the proper place such that all nodes below
         the start index are in heap order)
        siftDown(a, start, count)
    (after sifting down the root all nodes/elements are in heap order)
    
(Repair the heap whose root element is at index 'start', assuming the heaps rooted at its children are valid)
procedure siftDown(a, root, end) is
    while iLeftChild(root) < end do    (While the root has at least one child)
        child ← iLeftChild(root)       (Left child of root)
        (If there is a right child and that child is greater)
        if child+1 < end and a[child] < a[child+1] then
            child ← child + 1
    
        if a[root] < a[child] then
            swap(a[root], a[child])
            root ← child         (repeat to continue sifting down the child now)
        else
            (The root holds the largest element. Since we may assume the heaps rooted
             at the children are valid, this means that we are done.)
            return
*/
