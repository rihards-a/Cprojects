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

/* task:
Apraksts

Lielā pilsētā (vairāki miljoni iedzīvotāju, bet ne vairāk par miljardu) atrodas viena vienīga frizētava, kurā strādā neliels daudzums frizieru [1..9]. Katram frizierim ir savs numurs [1..9], lai vieglāk varētu organizēt frizētavas darbu. Frizētavā laiku skaita nosacītās laika vienībās [1 .. 2 000 000 000], un skaitīšana ir sākta kopš frizētavas atvēršanas brīža.

Kaut arī pilsētā klientu ir ļoti daudz un frizieri ir ļoti pieprasīti, katram frizierim obligāti ir jādodas atpūsties. Obligātais atpūtas laiks frizierim ir tad, kad laika momenta simtnieku pozīcijas cipars sakrīt ar friziera numuru, t.i. frizierim ar numuru 5 jāatpūšas laikos [500..599], [1500..1599], [2500..2599], utt. Pārtraukuma laikā frizierim ir kategoriski aizliegts apkalpot klientu. Bez tam klienta apkalpošanu nedrīkst sadalīt vairākos etapos, t.i. apkalpot drīkst tikai viens frizieris un bez jebkādiem pārtraukumiem. Tātad frizieris nedrīkst sākt apkalpot klientu, ja to nevar paspēt līdz pārtraukuma brīdim.

Klients ir jāapkalpo nekavējoties, ja eksistē brīvs frizieris un tam nav nekādu ierobežojumu veikt šo darbu. Frizierim, beidzot darbu pie kārtējā klienta, ar nākošo laika momentu jāmēģina apkalpot kādu klientu. Precīzāka specifikācija: klients K1 ierodas laikā T1 un apkalpošanai nepieciešams laiks A1. Ir Brīvs frizieris F1. Līdz ar to apkalpošana notiek laika intervālā [T1..T1+A1−1]. Apkalpošanas beigas ir laika momentā T1+A1−1. Ja klients K2 jau ir ieradies pirms vai tieši laika momentā T1+A1, tad frizieris F1 var sākt apkalpošanu klientam K2 laika momentā T1+A1.

Klienti gaida stingrā (godīgā) rindā. Vienā laika momentā var ierasties tikai viens klients. Reģistrators uzreiz nosaka klientam ierašanās kārtas numuru [1 .. 200 000] un pakalpojuma laiku [1..900].

Ja eksistē neapkalpots klients un uz to vienlaicīgi pretendē vairāki brīvi frizieri, tad:

    priekšroka ir frizierim, kurš visilgāk ir bijis bez klientiem (mēra no klienta apkalpošanas beigām);
    ja frizieriem ir vienāds gaidīšanas laiks, tad priekšroka ir frizierim ar mazāko numuru.

Zinot klientu ierašanās laikus, klientu kārtas numurus un pakalpojumu ilgumus, izdrukāt klientu apkalpošanas beigu momentus, apkalpojošā friziera numuru un klienta kārtas numuru. Drukāšana jāveic laika pieaugšanas secībā. Ja vienā laika momentā beidz apkalpot vairākus klientus, tad rezultāti jādrukā frizieru numuru secībā.
Ieeja:

Pirmajā rindiņā ir vesels skaitlis, kas nosaka fricieru skaitu. Tālāk seko informācija par klientiem to ierašanās secībā.

Frizieri
Laiks Klients Ilgums
...
0

    Frizieri nosaka frizieru skaitu [1..9]
    Laiks nosaka laika momentu, kad klients ieradies un ir jau gatavs apkalpošanai [1 .. 2 000 000 000]
    Klients nosaka klienta kārtas numuru [1 .. 200 000]
    Ilgums nosaka nepieciešamā pakalpojuma ilgumu [1..900]
    0 nosaka, ka ieejas dati ir beigušies. Šajā gadījumā Klients un Ilgums netiek norādīti

Ieejas dati ir stingrā hronoloģiskā secībā.

Ieejas dati ir korekti saskaņā ar ieejas formātu un dotajiem ierobežojumiem.
Izeja:

Atbilstoši ieejas failam katra klienta apkalpošanas beigu ieraksts ir formātā:

Laiks Frizieris Klients

    Laiks nosaka laika momentu, kad ir pēdējais klienta apkalpošanas moments [1 .. 2 000 000 000]
    Frizieris nosaka frizieri, kas apkalpoja klientu, t.i. tas ir friziera numurs [1..9]
    Klients nosaka klienta, kuru apkalpoja, kārtas numuru [1 .. 200 000]

Rezultātos neviens apkalpošanas beigu laiks nebūs lielāks par 2 000 000 000.
Piemērs:

Ieejas faila hair.in saturs:

2
11 1 10
21 2 50
31 3 20
0

Izejas faila hair.out saturs:

20 1 1
50 1 3
70 2 2

Papildus skaidrojumi:
Klienti ir jāapkalpo ievērojot stingru rindu

Nedrīkst būt situācija, kad kādu klientu sāk apkalpot ātrāk kā kādu pirms tam ieradušos klientu.
Viens frizieris

Ieejas faila hair.in saturs:

1
10 1 100
20 2 10
0

Izejas faila hair.out saturs:

299 1 1
309 1 2

Divi frizieri konkurē laikā brīdī 300

Ieejas faila hair.in saturs:

2
110 1 100
120 2 10
0

Izejas faila hair.out saturs:

299 1 1
309 2 2

Divi frizieri

Ieejas faila hair.in saturs:

2
110 1 99
120 2 10
0

Izejas faila hair.out saturs:

298 1 1
308 1 2

Vairākus klientus var sākt apkalpot vienlaicīgi

Ir atļauts vairākus klientu sākt apkalpot vienlaicīgi. Tas netiek uzskatīts par rindas pārkāpumu.
Divi frizieri atbrīvojas vienlaicīgi un ir gaidoši klienti

Ieejas faila hair.in saturs:

2
1 1 10
2 2 9
3 3 10
4 4 10
0

Izejas faila hair.out saturs:

10 1 1
10 2 2
20 1 3
20 2 4

*/
