/*
anniversary.c - Jubileja
Implementation notes:

Use a singular FNV linear probing hashmap of the form:
    (month,day) -> person
The hashmap doesn't include resizing or deletion functionality,
as per the task specification no deletions are intended. 
Hashmap size = 2^14 = 16384, which is a good fit for 10000 entries, 
leaving ~40% of space free.

When inserting new persons in the hashmap, in case the key matches
an existing one, instead of replacing/updating the value, we add
it to the nearest available slot with linear probing - we know it's
not a duplicate because of previous checks. Thanks to this we can 
quickly lookup all people with the same birthday, and also handle
collisions without needing to implement a more complex data structure.

Use a secondary array of all persons, pointed to by the hashmap.

When finding the nearest celebration, we can iterate through the days
of the year, since the space is relatively small (365 days), and look 
up each day in the hashmap, then probe for people with that date, then 
extract them from the secondary array, order them, and print them.

Ordering is done using heapsort, with a special less_than function
for Person objects, and also uses a case insensitive str_cmp.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_PERSONS 10000 
#define SHT_INIT_CAP 16384

typedef struct {
    char* first_name;
    char* last_name;
    char* date;
} Person;

/* =========================================================================
Minimal string helpers  (avoid pulling in <string.h>)
========================================================================= */

/* Compare two NUL-terminated strings for equality (string equals). */
static bool seq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

/* Heap-duplicate a NUL-terminated string (replacement for strdup). */
static char *sdup(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    char *p = (char*)malloc(n + 1);
    if (!p) return NULL;
    for (size_t i = 0; i <= n; i++) p[i] = s[i];
    return p;
}

/*  Compares 2 strings case-insensitively, returning:
returns -1 if a is smaller than b
returns  1 if b is smaller than a
returns  0 if they're equal */
static int case_insensitive_seq(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
        if (ca != cb) return (ca < cb) ? -1 : 1;
        a++; b++;
    }
    if (*a == *b) return 0;
    /* 'smaller' also includes having a smaller length */
    return (*a == '\0') ? -1 : 1;
}

/* =========================================================================
FNV-1a 64-bit hash  (fast and simple)
=========================================================================
See: https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function */

#define FNV_OFFSET  14695981039346656037ULL
#define FNV_PRIME   1099511628211ULL

/* For the size of our hashmap it's not necessary to use 64 bits, but 
it can help randomize the results, and doesn't do harm. */
static uint64_t fnv_str(const char *k) {
    uint64_t h = FNV_OFFSET;
    for (; *k; ++k) { h ^= (unsigned char)*k; h *= FNV_PRIME; }
    return h;
}


/* =========================================================================
String hash map  ("dd.mm")  ->  Person*)
=========================================================================
Keys are heap-owned by the map; sht_free() releases them.
The empty-slot sentinel for the key is NULL (pointer). */

typedef struct { const char *k; Person *v; } SE; /* string entry - one slot */
typedef struct { SE *e; size_t cap, len; } SHT;  /* string hash table */

/* Allocate a new, empty string hash map. */
static SHT *sht_new(void) {
    SHT *t = (SHT*)malloc(sizeof *t);
    if (!t) return NULL;
    t->cap = SHT_INIT_CAP;
    t->len = 0;
    t->e   = (SE*)calloc(t->cap, sizeof(SE));  /* calloc zeros -> all keys NULL = empty */
    if (!t->e) { free(t); return NULL; }
    return t;
}

/* Free the map and all heap-owned keys. */
static void sht_free(SHT *t) {
    for (size_t i = 0; i < t->cap; i++) free((void *)t->e[i].k);
    free(t->e);
    free(t);
}

/* Look up key; return associated persons' pointers in an array of size MAX_PERSONS or NULL if not found */
static Person **sht_get(SHT *t, const char *k) {
    int count = 0;
    Person **results = (Person**)malloc(MAX_PERSONS * sizeof(Person*));
    if (!results) return NULL;
    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_str(k) & (uint64_t)m);
    while (t->e[i].k) {
        if (seq(t->e[i].k, k)) {
            results[count++] = t->e[i].v;
        }
        i = (i + 1) & m;   /* linear probe; & m is modulo for power-of-2 cap */
    }
    if (count > 0) {
        results[count] = NULL; /* null-terminate the results array */
        return results;
    }
    free (results);
    return NULL;
}

/* Insert (k, v) in the first available slot. Returns false on allocation failure. */
static bool sht_set(SHT *t, const char *k, Person *v) {
    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_str(k) & (uint64_t)m);
    while (t->e[i].k) { i = (i + 1) & m; } /* find empty slot */
    /* New entry: heap-duplicate the key so the map owns it. */
    char *kc = sdup(k);
    if (!kc) return false;
    t->e[i].k = kc;
    t->e[i].v = v;
    t->len++;
    return true;
}

/* =========================================================================
Heapsort for ordering output
=========================================================================

/* returns ( a < b ) - 'does a come before b?' with Person property comparisons:
first, the oldest employees;
if the age is the same, then in alphabetical order by last name;
if the last names are also the same, then in alphabetical order by first name. */
static int result_less(const Person *a, const Person *b) {
    int birth_year_a, birth_year_b, comp;
    sscanf(a->date, "%*d.%*d.%4d", &birth_year_a); /* asterisks skip the entries */
    sscanf(b->date, "%*d.%*d.%4d", &birth_year_b);
    if (birth_year_a != birth_year_b) {
        return birth_year_a < birth_year_b; /* older first / smaller year first */
    }
    if ((comp = case_insensitive_seq(a->last_name, b->last_name)) != 0) {
        return comp < 0;
    }
    /* can't have duplicate persons, so safe to return this value */
    return case_insensitive_seq(a->first_name, b->first_name) < 0;
}

static void sift_down(Person **a, unsigned int root, unsigned int end) {
    while (2 * root + 1 < end) { /* while root has a child */
        unsigned int child = 2 * root + 1; /* property of binary trees in an array */
        if (child + 1 < end && result_less(a[child], a[child + 1])) {
            child++;
		}
        if (result_less(a[root], a[child])) {
            Person *tmp = a[root]; a[root] = a[child]; a[child] = tmp;
            root = child; /* continue sifting down to children */ 
        } else {
            return;
        }
    }
}

static void heapify(Person **a, unsigned int count) {
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

static void heapsort_persons(Person **a, unsigned int count) {
    heapify(a, count);
    unsigned int end = count;
	/* move the largest value - the first one to the very back
	   effectively sorting everything in ascending order */
    while (end-- > 1) {
        Person *tmp = a[end]; a[end] = a[0]; a[0] = tmp;
        sift_down(a, 0, end);
    }
}


/* =========================================================================
Other helper functions
========================================================================= */

static void get_next_date(int *day, int *month) {
    static const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    (*day)++;
    if (*day > days_in_month[*month - 1]) {
        *day = 1;
        (*month)++;
        if (*month > 12) {
            *month = 1;
        }
    }
}


/* =========================================================================
Main
========================================================================= */

int main(void) {
    if (!freopen("anniversary.in",  "r", stdin))  { perror("anniversary.in");  return 1; }
    if (!freopen("anniversary.out", "w", stdout)) { perror("anniversary.out"); return 1; }

    char cmd;
    SHT *sht = sht_new();

    while (scanf("%c", &cmd) == 1) {
        /* -------------------------------------------------------------------
        PERSON - INSERT
        ------------------------------------------------------------------- */
        if (cmd == 'P') {
            bool duplicate = false;
            char first_name[31];
            char last_name[31];
            char date[11];      /* "dd.mm.yyyy" */
            char day_month[6];  /* "dd.mm" */
            if (scanf(" %30s %30s %10s", first_name, last_name, date) != 3) {
                return 1;
            }
            sscanf(date, "%5s", day_month);

            /* check for dupliactes */
            Person **persons = sht_get(sht, day_month);
            if (persons) {
                for (int i = 0; i < MAX_PERSONS && persons[i]; i++) {
                    if (case_insensitive_seq(persons[i]->first_name, first_name) == 0 &&
                        case_insensitive_seq(persons[i]->last_name, last_name)   == 0) {
                        /* duplicate found - ignore it */
                        duplicate = true;
                        break;
                    }
                }
            }
            if (duplicate) continue;

            /* create a new person and insert into hashmap with given data */
            Person *p = (Person*)malloc(sizeof(Person));
            if (!p) return 1;
            *p = (Person){ sdup(first_name), sdup(last_name), sdup(date) };
            if (!sht_set(sht, day_month, p)) return 1;

        /* -------------------------------------------------------------------
        DATE - QUERY
        ------------------------------------------------------------------- */
        } else if (cmd == 'D') {
            char date[11];
            int day, month, year;
            if (scanf(" %10s", date) != 1) {
                return 1;
            }
            /* %2d because the given range has trailing 0s */
            sscanf(date, "%2d.%2d.%4d", &day, &month, &year);

            /* cycle the dates */
            for (int i = 0; i < 365; i++) {
                char day_month[6];
                sprintf(day_month, "%02d.%02d", day, month);
                Person **persons = sht_get(sht, day_month);
                if (persons) {
                    printf("%02d.%02d.%04d\n", day, month, year);
                    /* sort the results and print them */
                    int p_count = 0;
                    while (p_count < MAX_PERSONS && persons[p_count]) p_count++;
                    heapsort_persons(persons, p_count);
                    for (int j = 0; j < p_count && persons[j]; j++) {
                        int age = year - atoi(persons[j]->date + 6); /* extract the years from date strings */
                        printf("%i %s %s\n", age, persons[j]->first_name, persons[j]->last_name);
                    }
                    break;
                }
                int prev_month = month;
                get_next_date(&day, &month);
                if (month < prev_month) year++; /* wrapped months -> next year */
            }
        }
    }

    return 0;
}
