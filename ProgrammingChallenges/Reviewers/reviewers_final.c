/*
reviewers.c - Recenzenti
Implementation notes:

There are two hash tables:
    ph : pseudonym -> reviewer index
    nh : name      -> reviewer index

Both tables use open addressing (no linked lists) with linear probing. 
They grow when the load factor reaches 75%.

Deletion uses backward-shift instead of tombstones. This keeps clusters
compact and avoids slow scans after many insert/delete operations.

Reviewer records live in a fixed pool, allowing for O(1) allocation and 
deallocation without heap fragmentation. Only taking up a few MB of memory:
10000 * (100 chars (bytes) + 50 * 4 bytes + 4 bytes) ~ 3MB, well under 20MB.

Backward-shift deletion:
After removing slot i, scan forward until an empty slot is found.
For each occupied slot j (in order), compute the key's true position k.
Move the entry back to i unless k lies in the cyclic interval (i, j],
meaning the entry is still reachable without the deleted slot.

Interval rules:
    if i <= j: keep entry if k > i && k <= j // usual case
    if i > j:  keep entry if k > i || k <= j // wrap-around case

After moving an entry to i, set i = j and continue scanning from j = i + 1.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
Given contants and limits from the problem description.
========================================================================= */

#define MAX_REVIEWERS   10000
#define MAX_PSEUDONYMS  50
#define MAX_NAME_LEN    100

/* Initial bucket counts / size for the two hash maps.  Must be powers of 2.
both maps will grow via doubling as needed (rid = reviewer id). */
#define UHT_INIT_CAP    128     /* pseudonym  -> rid  map */
#define SHT_INIT_CAP    64      /* name       -> rid  map */


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


/* =========================================================================
FNV-1a 64-bit hash  (fast and simple)
=========================================================================
Used for both the string map and the integer map.
See: https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function        */

#define FNV_OFFSET  14695981039346656037ULL
#define FNV_PRIME   1099511628211ULL

/* For the size of our hashmap it's not necessary to use 64 bits, but 
it can help randomize the results, and doesn't do harm. */
static uint64_t fnv_str(const char *k) {
    uint64_t h = FNV_OFFSET;
    for (; *k; ++k) { h ^= (unsigned char)*k; h *= FNV_PRIME; }
    return h;
}

/* For uint32 keys we run one round of FNV mixing. */
static uint64_t fnv_u32(uint32_t k) {
    uint64_t h = FNV_OFFSET;
    h ^= (uint64_t)k;
    h *= FNV_PRIME;
    return h;
}


/* =========================================================================
String hash map  (name  ->  void*)
=========================================================================
Keys are heap-owned by the map; sht_free() releases them.
The empty-slot sentinel for the key is NULL (pointer). */

typedef struct { const char *k; int v; } SE;   /* string entry - one slot */
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

/* Look up key; return associated reviewer index, or -1 if not found. */
static int sht_get(SHT *t, const char *k) {
    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_str(k) & (uint64_t)m);
    while (t->e[i].k) {
        if (seq(t->e[i].k, k)) return t->e[i].v;
        i = (i + 1) & m;   /* linear probe; & m is modulo for power-of-2 cap */
    }
    return -1;
}

/* Internal helper: insert (k, v) into a raw slot array without duplicating k
or updating len. Used during table resizes where keys are already owned. */
static void sht_raw(SE *e, size_t cap, const char *k, int v) {
    size_t m = cap - 1;
    size_t i = (size_t)(fnv_str(k) & (uint64_t)m);
    while (e[i].k) i = (i + 1) & m;
    e[i].k = k;
    e[i].v = v;
}

/* Double the map's capacity, rehashing all live entries. */
static bool sht_grow(SHT *t) {
    size_t nc = t->cap * 2;
    SE *ne = (SE*)calloc(nc, sizeof(SE));
    if (!ne) return false;
    for (size_t i = 0; i < t->cap; i++)
        if (t->e[i].k) sht_raw(ne, nc, t->e[i].k, t->e[i].v);
    free(t->e);
    t->e   = ne;
    t->cap = nc;
    return true;
}

/* Insert or update (k, v). Returns false on allocation failure. */
static bool sht_set(SHT *t, const char *k, int v) {
    /* Grow before hitting 75 % load to keep probing chains short. */
    if (t->len >= t->cap * 3 / 4 && !sht_grow(t)) return false;

    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_str(k) & (uint64_t)m);
    while (t->e[i].k) {
        if (seq(t->e[i].k, k)) { t->e[i].v = v; return true; }  /* update */
        i = (i + 1) & m;
    }
    /* New entry: heap-duplicate the key so the map owns it. */
    char *kc = sdup(k);
    if (!kc) return false;
    t->e[i].k = kc;
    t->e[i].v = v;
    t->len++;
    return true;
}

/* Delete key k from the map. Uses backward-shift to avoid tombstones.
Returns true if the key was present, false otherwise. */
static void sht_del(SHT *t, const char *k) {
    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_str(k) & (uint64_t)m);

    /* Find the slot holding k (or an empty slot meaning k is absent). */
    while (t->e[i].k && !seq(t->e[i].k, k)) i = (i + 1) & m;
    if (!t->e[i].k) return;   /* not found */

    free((void *)t->e[i].k);
    t->e[i].k = NULL;
    t->len--;

    /* Backward-shift: close the gap left by the deleted entry so subsequent
    linear probes still terminate correctly. */
    for (size_t j = (i + 1) & m; t->e[j].k; j = (j + 1) & m) {
        size_t nk   = (size_t)(fnv_str(t->e[j].k) & (uint64_t)m);
        bool   stay = (i <= j) ? (nk > i && nk <= j)
                               : (nk > i || nk <= j); /* wrap-around case */
        if (!stay) {
            t->e[i] = t->e[j];     /* slide entry back to fill the gap */
            t->e[j].k = NULL;
            i = j;                 /* new gap is now at j */
        }
    }
}

/* =========================================================================
Integer (uint32) hash map  (pseudonym  ->  int reviewer-pool-index)
=========================================================================
Key 0 is the empty-slot sentinel, which is safe because pseudonyms are
constrained to [1 … 1 000 000 000]. No heap allocation for keys.       */

typedef struct { uint32_t k; int v; } UE;   /* one slot; k==0 -> empty */
typedef struct { UE *e; size_t cap, live; } UHT;

/* Allocate a new, empty integer hash map. */
static UHT *uht_new(void) {
    UHT *t = (UHT*)malloc(sizeof *t);
    if (!t) return NULL;
    t->cap  = UHT_INIT_CAP;
    t->live = 0;
    t->e    = (UE*)calloc(t->cap, sizeof(UE));   /* zeros -> all k==0 = empty */
    if (!t->e) { free(t); return NULL; }
    return t;
}

static void uht_free(UHT *t) { free(t->e); free(t); }

/* Return the reviewer index for pseudonym k, or -1 if not found. */
static int uht_get(UHT *t, uint32_t k) {
    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_u32(k) & (uint64_t)m);
    while (t->e[i].k) {
        if (t->e[i].k == k) return t->e[i].v;
        i = (i + 1) & m;
    }
    return -1;
}

/* Double the map's capacity, rehashing all live entries. */
static bool uht_grow(UHT *t) {
    size_t nc = t->cap * 2;
    UE *ne = (UE*)calloc(nc, sizeof(UE));
    if (!ne) return false;
    size_t nm = nc - 1;
    for (size_t i = 0; i < t->cap; i++) {
        if (t->e[i].k) {
            size_t j = (size_t)(fnv_u32(t->e[i].k) & (uint64_t)nm);
            while (ne[j].k) j = (j + 1) & nm;
            ne[j] = t->e[i];
        }
    }
    free(t->e);
    t->e   = ne;
    t->cap = nc;
    return true;
}

/* Insert or update mapping k -> v.  Returns false only on alloc failure. */
static bool uht_set(UHT *t, uint32_t k, int v) {
    if (t->live >= t->cap * 3 / 4 && !uht_grow(t)) return false;
    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_u32(k) & (uint64_t)m);
    while (t->e[i].k) {
        if (t->e[i].k == k) { t->e[i].v = v; return true; }
        i = (i + 1) & m;
    }
    t->e[i].k = k;
    t->e[i].v = v;
    t->live++;
    return true;
}

/* Delete pseudonym k from the map using backward-shift deletion. */
static void uht_del(UHT *t, uint32_t k) {
    size_t m = t->cap - 1;
    size_t i = (size_t)(fnv_u32(k) & (uint64_t)m);

    while (t->e[i].k && t->e[i].k != k) i = (i + 1) & m;
    if (!t->e[i].k) return;   /* not present; nothing to do */

    t->e[i].k = 0;            /* mark slot empty (0 = sentinel) */
    t->live--;

    /* Backward-shift: restore the linear-probing invariant. */
    for (size_t j = (i + 1) & m; t->e[j].k; j = (j + 1) & m) {
        size_t nk   = (size_t)(fnv_u32(t->e[j].k) & (uint64_t)m);
        bool   stay = (i <= j) ? (nk > i && nk <= j)
                                : (nk > i || nk <= j);
        if (!stay) {
            t->e[i] = t->e[j];
            t->e[j].k = 0;
            i = j;
        }
    }
}


/* =========================================================================
Static reviewer pool  +  free-index stack
=========================================================================
We never heap-allocate individual reviewer records. All 10000 slots sit
in a single static array, which is cheap (~ 3 MB) and avoids fragmentation.

The free-index stack (fstk) tracks which slots are available. palloc()
pops a slot index in O(1); pfree() pushes it back. Deleted slots are
immediately recycled for future reviewers.                               */

typedef struct {
    char     name[MAX_NAME_LEN + 1];
    uint32_t keys[MAX_PSEUDONYMS];
    int      cnt; /* number of pseudonyms stored */
} Rev;

static Rev  pool[MAX_REVIEWERS];
static int  fstk[MAX_REVIEWERS];   /* stack of free slot indices */
static int  ftop;                  /* stack pointer / counter */

/* Initialise the free stack so every slot is available. */
static void pool_init(void) {
    ftop = MAX_REVIEWERS;
    for (int i = 0; i < MAX_REVIEWERS; i++)
        fstk[i] = MAX_REVIEWERS - 1 - i;   /* push in reverse; order doesn't matter */
}

/* Pop a free slot index, or return -1 if the pool is exhausted. */
static int palloc(void) {
    return ftop > 0 ? fstk[--ftop] : -1;
}

/* Return slot rid to the pool for future reuse. */
static void pfree(int rid) {
    pool[rid].cnt = 0; /* wipe count so a recycled slot looks empty */
    fstk[ftop++] = rid;
}


/* =========================================================================
main  -  command dispatch loop
========================================================================= */

int main(void) {
    if (!freopen("reviewers.in",  "r", stdin))  { perror("reviewers.in");  return 1; }
    if (!freopen("reviewers.out", "w", stdout)) { perror("reviewers.out"); return 1; }

    pool_init();

    SHT *nh = sht_new();    /* name  -> reviewer index */
    UHT *ph = uht_new();    /* pseudonym -> reviewer index */
    if (!nh || !ph) return 1;

    char         cmd;
    char         name[MAX_NAME_LEN + 1];
    unsigned int cnt, key;

    /* scanf " %c" skips any leading whitespace (spaces, newlines, tabs)
    before reading the single command character. The following scans also
    have a trailling space for skipping whitespace, as the specification asks */
    while (scanf(" %c", &cmd) == 1) {
 
        /*  -------------------------------------------------------------------
            INSERT
            -------------------------------------------------------------------
            1. Read pseudonyms into buffer
            2. Check if reviewer exists already
            3. Classify each pseudonym/key as:
                free; owned by current reviewer; owned by different reviewer (conflict)
            4. Deduplicate keys already owned by current reviewer
            5. Verify whether it will exceed the MAX_PSEUDONYMS limit
            6. If all checks pass, commit changes
        */
        if (cmd == 'I') {
            if (scanf(" %100s %u", name, &cnt) != 2) break;
 
            /* Read pseudonyms into buffer */
            uint32_t buf[MAX_PSEUDONYMS];
            for (unsigned int i = 0; i < cnt && i < MAX_PSEUDONYMS; i++) {
                if (scanf(" %u", &key) != 1) break;
                buf[i] = (uint32_t)key;
            }
 
            /* Check if reviewer exists already */
            int rid = sht_get(nh, name);
            /* rid == -1  -> brand-new reviewer
               rid >= 0   -> existing reviewer being extended */
 
            /* Classify each incoming key and collect genuinely new ones. */
            bool fail = false;
            uint32_t new_keys[MAX_PSEUDONYMS];
            int nkc = 0;
 
            for (unsigned int i = 0; i < cnt && !fail; i++) {
                uint32_t k     = buf[i];
                int      owner = uht_get(ph, k);
 
                if (owner == -1) {
                    /* Key is globally free.  Deduplicate within new_keys. */
                    bool seen = false;
                    for (int j = 0; j < nkc; j++)
                        if (new_keys[j] == k) { seen = true; break; }
                    if (!seen) new_keys[nkc++] = k;
                 } else if (owner == rid) {
                    /* Key already belongs to this very reviewer.
                       The spec says duplicates in the input are possible; */
                 } else {
                    /* Key belongs to a different reviewer - conflict. */
                    fail = true;
                }
            }
 
            /* Capacity check: would the unique pseudonym count exceed 50? */
            if (!fail) {
                int current = (rid == -1) ? 0 : pool[rid].cnt;
                if (current + nkc > MAX_PSEUDONYMS) fail = true;
            }
 
            if (fail) { puts("no"); continue; }
 
            /* All checks passed - commit changes. */
             if (rid == -1) {
                /* New reviewer: allocate a pool slot. */
                rid = palloc();
                if (rid == -1) { puts("no"); continue; }  /* pool full */
 
                /* Copy name into the pool record. */
                int ni = 0;
                while (name[ni]) { pool[rid].name[ni] = name[ni]; ni++; }
                pool[rid].name[ni] = '\0';
                pool[rid].cnt = 0;
 
                /* Register name in the string map. */
                if (!sht_set(nh, name, rid)) {
                    pfree(rid); /* roll back when out of memory */
                    puts("no");
                    continue;
                }
            }
 
            /* Append new pseudonyms to the pool record and the integer map. */
            for (int i = 0; i < nkc; i++) {
                uht_set(ph, new_keys[i], rid);
                pool[rid].keys[pool[rid].cnt++] = new_keys[i];
            }
 
            puts("ok");
 
        /*  -------------------------------------------------------------------
            DELETE
            -------------------------------------------------------------------
            Find the reviewer via their pseudonym, remove every one of their
            pseudonyms from ph, remove their name from nh, then recycle the
            pool slot (ph = pseudonym hashtable; nh = name hastable).
        */
        } else if (cmd == 'D') {
            if (scanf(" %u", &key) != 1) break;
 
            int rid = uht_get(ph, (uint32_t)key);
            if (rid == -1) { puts("no"); continue; }
 
            /* Remove all pseudonyms belonging to this reviewer. */
            for (int i = 0; i < pool[rid].cnt; i++)
                uht_del(ph, pool[rid].keys[i]);
 
            /* Remove the name entry. */
            sht_del(nh, pool[rid].name);
 
            /* Recycle the pool slot (also resets cnt to 0). */
            pfree(rid);
 
            puts("ok");

        /*  -------------------------------------------------------------------
            LOOKUP
            -------------------------------------------------------------------
            Find the reviewer via their pseudonym, return their name from the pool
        */
        } else if (cmd == 'L') {
            if (scanf(" %u", &key) != 1) break;
 
            int rid = uht_get(ph, (uint32_t)key);
            if (rid == -1) { puts("no"); continue; }
 
            puts(pool[rid].name);
        }
    }

    /* Free heap memory (good practice; OS would reclaim it anyway). */
    uht_free(ph);
    sht_free(nh);
    return 0;
}


/*
Programming Task: Reviewers
Memory: 20 MB
Time: 0.5 seconds
Input file: reviewers.in
Output file: reviewers.out
Description

The editor-in-chief of the journal "All About Science" noticed one day that there were problems with ensuring 
high-quality reviews of all submitted articles. In recent years, science has been developing rapidly in all fields; 
there are many articles, and consequently, many reviewers associated with the editorial board. 
Managing all reviewers without a computerized system has become difficult.

It was decided to create an electronic database to replace the existing card system. 
Each reviewer has a unique name consisting of a string of symbols {a..z, A..Z, 0..9} of length [1..100]. 
To ensure greater anonymity, each reviewer has [1..50] unique pseudonyms. 
Numbers in the range [1..1’000’000’000] were used for the pseudonyms. 
The same pseudonym cannot be assigned to different reviewers; that is, at any given time, 
a specific reviewer can be identified by any of the pseudonyms. 
The journal’s editorial board decided that there would be no more than 10,000 reviewers at any one time.

Since the record-keeping had not been accurate up to that point, it happened that a single reviewer’s pseudonym was 
listed multiple times in the old card system (duplicates in the insertion command), and information about the reviewer 
had to be entered in multiple passes (there may be multiple entry commands for a single reviewer). Pseudonyms are 
not sorted in any way within the entry command, as the old cards were not sorted before being entered into the new system.

An electronic database must be created to enable the following actions regarding reviewers:
    adding reviewers or updating their list of pseudonyms (new reviewers are added, 
        or the list of pseudonyms for an already registered reviewer is updated);
    removal (termination of a reviewer’s services);
    searching (finding a reviewer’s unique name if one of their pseudonyms is known).
After a reviewer is removed from the database, new reviewers can once again use pseudonyms that were previously assigned to a reviewer.
Your task is to program the core of this database, which performs all operations very quickly ( ~O(log(n)) ).
The input is a file that simulates real-life events, i.e., commands to the file system. 
Each command with its parameters is written on a separate line. 
Each line of the file may contain as many trailing spaces as desired. 
The file size is unlimited. The input data is valid according to the input data format and the given constraints.


Input and Output (File System Commands and Operations)

Insertion or Appending to the List of Pseudonyms
The input is a line in the format:
I name count key_1 ... key_count
    I is an abbreviation for Insert
    name is the reviewer’s unique name
    count is the number of aliases [1..50] assigned to the reviewer
    key_1 ... key_count are all aliases separated by spaces
Output a line containing:
    the word “ok” if the reviewer was successfully added or the list of pseudonyms for an existing reviewer was successfully updated,
ok
    the word “no” if the reviewer cannot be added because a pseudonym is already in use
no

Removing a reviewer
Input a line in the following format:
D key
    D is short for Delete
    key is one of the pseudonyms of the reviewer to be removed
Output a line containing:
    the word “ok” if the reviewer has been successfully removed
    ok
    the word “no” if the reviewer cannot be removed because there is no reviewer with the specified pseudonym
    no

Searching for a reviewer
Input: a line in the format:
L key
    L is short for Look_up
    key is one of the nicknames of the reviewer to be searched for
Output: a line containing:
    the reviewer's name, if the reviewer is successfully found,
    Name
    the word “no” if the reviewer cannot be found because there is no reviewer with the specified pseudonym.
    no

Example:
Content of the input file reviewers.in:
I JackSmart 3 9 1009 1000009
L 1000009
I TedPumpkinhead 1 19
I PeterMeter 1 9
L 19
D 19
L 19
I JohnCritic 2 1 19
L 19
L 1
L 9
Content of the output file reviewers.out:
ok
JackSmart
ok
no
TedPumpkinhead
ok
no
ok
JohnCritic
JohnCritic
JackSmart
*/
