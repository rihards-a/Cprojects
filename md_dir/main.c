#include <dirent.h>     // DIR, struct dirent, opendir, readdir, closedir
#include <sys/stat.h>   // struct stat, S_ISDIR, lstat, stat, st_size, st_mtime
#include <string.h>     // memmove, memcpy, strlen
#include <stdio.h>      // printf
#include <limits.h>     // PATH_MAX
#include <time.h>       // localtime, time_t, struct tm, strftime

#include <openssl/evp.h>  // MD5 that isn't flagged as deprecated - https://linux.die.net/man/3/evp_md5

// ---------------------------------------------------------------------------
// Simple hash table implemented in C.
// ---------------------------------------------------------------------------
// credit to https://github.com/benhoyt/ht
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
// #include <string.h>
#include <stdbool.h>
#include <stddef.h>

// Hash table entry (slot may be filled or empty).
typedef struct {
    const char* key;  // key is NULL if this slot is empty
    void* value;
} ht_entry;

// Hash table structure: create with ht_create, free with ht_destroy.
typedef struct ht {
    ht_entry* entries;  // hash slots
    size_t capacity;    // size of _entries array
    size_t length;      // number of items in hash table
} ht;

// Hash table iterator: create with ht_iterator, iterate with ht_next.
typedef struct {
    const char* key;  // current key
    void* value;      // current value

    // Don't use these fields directly.
    ht* _table;       // reference to hash table being iterated
    size_t _index;    // current index into ht._entries
} hti;

#define INITIAL_CAPACITY 16  // must not be zero

ht* ht_create(void) {
    // Allocate space for hash table struct.
    ht* table = malloc(sizeof(ht));
    if (table == NULL) {
        return NULL;
    }
    table->length = 0;
    table->capacity = INITIAL_CAPACITY;

    // Allocate (zero'd) space for entry buckets.
    table->entries = calloc(table->capacity, sizeof(ht_entry));
    if (table->entries == NULL) {
        free(table); // error, free table before we return!
        return NULL;
    }
    return table;
}

void ht_destroy(ht* table) {
    // First free allocated keys.
    for (size_t i = 0; i < table->capacity; i++) {
        free((void*)table->entries[i].key);
    }

    // Then free entries array and table itself.
    free(table->entries);
    free(table);
}

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static uint64_t hash_key(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

void* ht_get(ht* table, const char* key) {
    // AND hash with capacity-1 to ensure it's within entries array.
    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(table->capacity - 1));

    // Loop till we find an empty entry.
    while (table->entries[index].key != NULL) {
        if (strcmp(key, table->entries[index].key) == 0) {
            // Found key, return value.
            return table->entries[index].value;
        }
        // Key wasn't in this slot, move to next (linear probing).
        index++;
        if (index >= table->capacity) {
            // At end of entries array, wrap around.
            index = 0;
        }
    }
    return NULL;
}

// Internal function to set an entry (without expanding table).
static const char* ht_set_entry(ht_entry* entries, size_t capacity,
        const char* key, void* value, size_t* plength) {
    // AND hash with capacity-1 to ensure it's within entries array.
    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(capacity - 1));

    // Loop till we find an empty entry.
    while (entries[index].key != NULL) {
        if (strcmp(key, entries[index].key) == 0) {
            // Found key (it already exists), update value.
            entries[index].value = value;
            return entries[index].key;
        }
        // Key wasn't in this slot, move to next (linear probing).
        index++;
        if (index >= capacity) {
            // At end of entries array, wrap around.
            index = 0;
        }
    }

    // Didn't find key, allocate+copy if needed, then insert it.
    if (plength != NULL) {
        key = strdup(key);
        if (key == NULL) {
            return NULL;
        }
        (*plength)++;
    }
    entries[index].key = (char*)key;
    entries[index].value = value;
    return key;
}

// Expand hash table to twice its current size. Return true on success,
// false if out of memory.
static bool ht_expand(ht* table) {
    // Allocate new entries array.
    size_t new_capacity = table->capacity * 2;
    if (new_capacity < table->capacity) {
        return false;  // overflow (capacity would be too big)
    }
    ht_entry* new_entries = calloc(new_capacity, sizeof(ht_entry));
    if (new_entries == NULL) {
        return false;
    }

    // Iterate entries, move all non-empty ones to new table's entries.
    for (size_t i = 0; i < table->capacity; i++) {
        ht_entry entry = table->entries[i];
        if (entry.key != NULL) {
            ht_set_entry(new_entries, new_capacity, entry.key,
                         entry.value, NULL);
        }
    }

    // Free old entries array and update this table's details.
    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
    return true;
}

const char* ht_set(ht* table, const char* key, void* value) {
    assert(value != NULL);
    if (value == NULL) {
        return NULL;
    }

    // If length will exceed half of current capacity, expand it.
    if (table->length >= table->capacity / 2) {
        if (!ht_expand(table)) {
            return NULL;
        }
    }

    // Set entry and update length.
    return ht_set_entry(table->entries, table->capacity, key, value,
                        &table->length);
}

size_t ht_length(ht* table) {
    return table->length;
}

hti ht_iterator(ht* table) {
    hti it;
    it._table = table;
    it._index = 0;
    return it;
}

bool ht_next(hti* it) {
    // Loop till we've hit end of entries array.
    ht* table = it->_table;
    while (it->_index < table->capacity) {
        size_t i = it->_index;
        it->_index++;
        if (table->entries[i].key != NULL) {
            // Found next non-empty item, update iterator key and value.
            ht_entry entry = table->entries[i];
            it->key = entry.key;
            it->value = entry.value;
            return true;
        }
    }
    return false;
}
// ---------------------------------------------------------------------------
// I would've put this in a header, but per the task conditions I can only submit main.c
// ---------------------------------------------------------------------------

static char CPATH[PATH_MAX];    // global buffer for constructing the (currently active) path
ht* GLOBAL_TABLE;               // global hash table for storing files
bool CHECK_DATE = false;
bool CHECK_MD5 = false;

// could use an array of linked lists for tracking duplicates, but easier to reuse hashmap and parse it once
typedef struct path_ll {
    char path[PATH_MAX]; // can malloc, but should have enough memory
    struct path_ll* next;
} path_ll;

void append_path(struct path_ll** start, const char* path)
{
    if (!start || !path) {return;}

    struct path_ll* node = malloc(sizeof *node); // same as malloc(sizeof(struct path_ll))
    if (!node) {return;}

    strncpy(node->path, path, PATH_MAX - 1);
    node->path[PATH_MAX - 1] = '\0';
    node->next = NULL;

    if (!*start) {
        *start = node;
        return;
    }

    struct path_ll *cur = *start;
    while (cur->next) {
        cur = cur->next;
    }

    cur->next = node;
    return;
}

// the initial value contains the name, date and hash based off flags
void update_key_ll(const char* key, const char* inital_value) {
    path_ll* ll = ht_get(GLOBAL_TABLE, key);
    if (ll) {
        append_path(&ll, CPATH); // update the linked list or create a new one if NULL
    } else {
        append_path(&ll, inital_value); // add the initial value to the linked list
        append_path(&ll, CPATH);
        ht_set(GLOBAL_TABLE, key, ll); // initialize the linked list for this key in the hash table
    }
}

void recurse_dir(const char* dirpath) 
{
    DIR* dir = opendir(dirpath);
    if (!dir) {return;}

    struct dirent* de;
    struct stat st;
    size_t dir_len = strlen(dirpath);
    memcpy(CPATH, dirpath, dir_len+1); // +1 for null terminator
    if (CPATH[dir_len-1] != '/') {
        memmove(CPATH + dir_len, "/", 2); // add trailing slash for directories
        dir_len++;
    }

    while ((de = readdir(dir))) {
        const char* name = de->d_name;
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {continue;} // skip "." and ".." entries
        
        memmove(CPATH + dir_len, name, strlen(name)+1); // append the entry name
        
        if (lstat(CPATH, &st) == -1) { // lstat to avoid following symlinks
            goto cont;
        }

        if (S_ISDIR(st.st_mode)) {
            recurse_dir(CPATH);
        } else {
            if (CHECK_MD5) { // read the file in chunks and compute the MD5 hash
                FILE* file = fopen(CPATH, "rb");
                if (!file) {
                    printf("Kļūda: nevar atvērt failu '%s' MD5 aprēķinam.\n", CPATH);
                    goto cont;
                }
                EVP_MD_CTX* ctx = EVP_MD_CTX_new();
                if (!ctx) {
                    printf("Kļūda veidojot EVP (\"EnVeloPe\" OpenSSL bibliotēka) kontekstu.\n");
                    goto cont;
                }
                if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1) {
                    printf("Kļūda inicializējot EVP kontekstu MD5 aprēķinam.\n");
                    goto cont;
                }
                unsigned char buffer[8192];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    if (EVP_DigestUpdate(ctx, buffer, bytes) != 1) {
                        printf("Kļūda atjaunot MD5.\n");
                        goto cont;
                    }
                }
                unsigned char digest[EVP_MAX_MD_SIZE];
                unsigned int digest_len;
                if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
                    printf("Kļūda aprēķinot pēdējo MD5 summu.\n");
                    goto cont;
                }
                EVP_MD_CTX_free(ctx);
                fclose(file);

                char hex[EVP_MAX_MD_SIZE * 2 + 1]; // change digest to hex to fix null-byte issues
                for (unsigned int i = 0; i < digest_len; i++) {
                    sprintf(hex + i * 2, "%02x", digest[i]);
                }
                
                if (CHECK_DATE) { // if -d -m flags - compute hash of full content (including name and date)
                    time_t mtime = st.st_mtime;
                    struct tm *tm_info = localtime(&mtime);
                    char buf[20];
                    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_info);
                    char combined[PATH_MAX + 20 + EVP_MAX_MD_SIZE * 2 + 100]; // path_name + date + md5 + enough for file size
                    snprintf(combined, sizeof(combined), "%s %d %s %s", buf, (int)st.st_size, name, hex);
                    update_key_ll((const char*)combined, combined);
                } else { // if -m flag - compute hash of content only (no name and date)
                    update_key_ll((const char*)hex, (const char*)hex);
                }
            } else {
                if (CHECK_DATE) { // if -d flag - compute hash of name+size+date
                    time_t mtime = st.st_mtime;
                    struct tm *tm_info = localtime(&mtime);
                    char buf[20];
                    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_info);
                    char combined[PATH_MAX + 20 + 100];
                    snprintf(combined, sizeof(combined), "%s %d %s", buf, (int)st.st_size, name);
                    update_key_ll((const char*)combined, combined);
                } else { // if no flags - compute hash of name+size
                    char combined[PATH_MAX + 100];
                    snprintf(combined, sizeof(combined), "%d %s", (int)st.st_size, name);
                    update_key_ll((const char*)combined, combined);
                }
            }
        }
        cont:
            CPATH[dir_len] = '\0'; // reset path for the next file
    }

    closedir(dir);
}

void print_help() {
    printf("Izsaukšana: md3 [-d | -m | -h]\n");
    printf("Apstaigā esošās direktorijas koku un izprintē duplikātu faila atrašanās vietas, ja tādi ir.\n");
    printf("Faili uzskatāmi par vienādiem, ja sakrīt izmērs un nosaukums, izņemot MD5 režīmā, kad salīdzina visu pārējo.\n");
    printf("Režīmi:\n");
    printf("\t-d: pārbauda arī satura izmaiņu datumus\n");
    printf("\t-m: aprēķina MD5 vērtību saturam, neiekļaujot vārdu un datumu\n");
    printf("\t-h: izvada šo palīgtekstu.\n");
    printf("Izvades formāts:\n");
    printf("=== datums izmērs nosaukums MD5\n");
}

int main(int argc, char **argv) 
{
    if (argc > 4) {
        print_help();
        return -1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0) {
            CHECK_DATE = true;
        } else if (strcmp(argv[i], "-m") == 0) {
            CHECK_MD5 = true;
        } else {
            print_help();
            return -1;
        }
    }

    const char* dirarg = "./";
    struct stat st;

    if (stat(dirarg, &st) == -1 || !S_ISDIR(st.st_mode)) {
        printf("Kļūda: '%s' nav derīga direktorija.\n", dirarg);
        return -1;
    }

    GLOBAL_TABLE = ht_create();

    recurse_dir(dirarg);

    // parse the hash table and print duplicates
    hti it = ht_iterator(GLOBAL_TABLE);
    while (ht_next(&it)) {
        path_ll* ll = it.value;
        if (ll && ll->next && ll->next->next) { // only print if there are duplicates
            printf("=== %s\n",  ll->path);
            for (path_ll* cur = ll->next; cur; cur = cur->next) {
                printf("%s\n", cur->path+2);
            }
            printf("\n");
        }
    }
    ht_destroy(GLOBAL_TABLE);

    return 0;
}

/*
Uzdevums

Uzrakstīt programmu md3, kas apstaigā direktoriju koku un atrod tos failus kas ir duplikāti, respektīvi, atrodami vairākos eksemplāros.

Apstaigāšana jāsāk no tās direktorijas, kurā programma izpildās (t.i. direktorijas ar nosaukumu ".").

Faili tiek uzskatīti par vienādiem, ja tiem ir vienāds izmērs un faila vārds, izņemot MD5 režīmā, kad failu vienādību nosaka MD5 vērtības.

Saites (symbolic links, t.i. "vājās" saites) jāignorē. Divas vājās saites, vai arī fails un vājā saite uz to nav jāsalīdzina.

Vārds un izmērs jāpārbauda vienmēr, izņemot MD5 režīmu. Papildus pārbaudes nosaka parametri:

md3 -d      pārbauda arī faila satura izmaiņu datumu (st_mtime) sakritību
md3 -m      aprēķina un salīdzina MD5 vērtību faila saturam (bez vārda un datuma). 
md3 -h      izvada palīga tekstu par parametriem

Izdrukas formāts (uz stdout):

=== date size filename1 [MD5]
path1/filename1 
path2/filename1 
...

=== date size filename2 [MD5]
path1/filename2 
path2/filename2 
...

Datuma formāts ir tāds pat kā ls -ll: (yyyy-mm-dd hh:mm). Piemēram: 2010-09-25 21:45

MD5 jādrukā tikai tad, ja pie programmas izsaukuma parametrs bija -m. MD5 drukājams kā heksadecimālu simbolu virkne bez tukšumiem.

Šajā gadījumā failu vienādību nosaka tikai MD5 sakritība, un izvads var nedaudz atšķirties - jo vienādiem failiem var atšķirties to vārdi un satura izmaiņu datumi. Datums izvadāms tikai vienam failam.

=== date size filename1 [MD5]
path1/filename1 
path2/filename2 
path3/filename3 
...

*/