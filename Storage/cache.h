#ifndef CACHE_H
#define CACHE_H
#include "ps.h"
#include "defs.h"
#include "../RW_lock/rwlock.h"
#include <limits.h> // for LONG_MAX

extern ENTRY *cache_ptr;
extern long CACHE_LEN;

// this will be used as return type from find_in_cache
struct entry_with_status {
    ENTRY *entry;
    int status;
};

void initialize_cache();
struct entry_with_status *find_update_cache_line(char *key, char *val, int req, int id);
void remove_from_cache(ENTRY *loc);
void update_cache_line(ENTRY *loc, char *key, char *val);
void update_frequency_timestamp(ENTRY *loc);

void initialize_cache() {
    cache_ptr = (ENTRY *)malloc(CACHE_LEN * sizeof(ENTRY));
    for (int i = 0; i < CACHE_LEN; i++) {
        ENTRY *ptr = cache_ptr + i;
        ptr->is_valid = 'F'; 
        ptr->is_dirty = 'F';
        init_rwlock(&(ptr->rwl));
        // printf("Initialised lock at %p\n", ptr);
    }
    // printf("Size of entry: %ld\n", sizeof(ENTRY));
}

void display_cache(){
    // for (int i = 0; i < CACHE_LEN; i++) {
    //     ENTRY *ptr = cache_ptr + i;
    //     display_chars(ptr->key,6);
    //     printf("--");
    //     display_chars(ptr->val,6);
    //     printf("\n");
    // }
}


/*
    Input : 
        key 
        val : NULL for req == 1 or req ==3
              value to be updated for req == 2
        req : this is the status code of request message ie. 1 for GET, 2 for PUT, 3 for DEL

    Return:
        This function returns the cache line entry along with a status
        Status = 1 -> key is found and corresponding cache line is returned
        Status = 2 -> an available Cache line is returned
        Status = 3 -> Cache line with LRU Key is returned
*/
struct entry_with_status *find_update_cache_line(char *key, char *val, int req, int id) {
    // printf("find_update_cache_line\n");
    int status = 3; //by default status = 3
    unsigned long oldest_timestamp = LONG_MAX;
    ENTRY *entry = NULL;
    for (int i = 0; i < CACHE_LEN; i++) {
        ENTRY *loc = cache_ptr + i;
        int j = loc-cache_ptr;
        // printf("Trying for write lock at %d with reader count: %d\n",j, loc->rwl.reader_count);
        write_lock(&(loc->rwl));
        // printf("Obtained write lock \n");
        if (loc->is_valid == 'T' && (strcmp(loc->key, key) == 0)) {
            // printf("(cache) Found: %s\n", loc->key);
            entry = loc;
            status = 1;
            if (req == 1) {
                update_frequency_timestamp(loc);
                ENTRY *temp = (ENTRY *)malloc(sizeof(ENTRY));
                
                // Take backup 
                // Reason: if some other thread changes this cache line's content, we don't fetch that value
                temp->key = loc->key;
                temp->val = loc->val;
                write_unlock(&(loc->rwl));
                // printf("Unlocked write lock at %d\n", j);
                struct entry_with_status *ret = (struct entry_with_status *)malloc(sizeof(struct entry_with_status));
                ret->entry = temp;
                ret->status = status;
                return ret;
            } else if (req == 2) {
                char *prev = strdup(loc->val);
                // printf("WT = %d: Prev = %s New = %s\n", id, loc->val, val);
                update_cache_line(loc, key, val);
                // printf("WT = %d: After update  = %s\n", id, loc->val);
                if (strcmp(prev, loc->val) == 0) {
                    // printf("WT = %d: Cache line not updated\nPrev: %s\nNew request: %s\n", id, prev, val);
                    // printf("Key: %s\nNew request Key: %s\n", loc->key, key);
                    
                    // exit(0);
                }
                write_unlock(&(loc->rwl));
                // printf("Unlocked write lock at %d\n", j);
                struct entry_with_status *ret = (struct entry_with_status *)malloc(sizeof(struct entry_with_status));
                ret->entry = NULL;
                ret->status = status;
                return ret;
            }
            else {
                remove_from_cache(loc);
                write_unlock(&(loc->rwl));
                // printf("Unlocked write lock at %d\n", j);
                struct entry_with_status *ret = (struct entry_with_status *)malloc(sizeof(struct entry_with_status));
                ret->entry = NULL;
                ret->status = status;
                return ret;
            }
        } else if (loc->is_valid == 'F') {
            // printf("At loc %p\n",loc);
            // printf("Status update to 2\n");
            
            status = 2;
            entry = loc;
        } else if (status!=2 && loc->is_valid == 'T' && oldest_timestamp > loc->timestamp) {
            // printf("At loc %p\n",loc);
            // printf("Updating LRU block\n");
            oldest_timestamp = (long int)loc->timestamp;
            entry = loc;
        }

        write_unlock(&(loc->rwl));
        // printf("Unlocked write lock at %d\n", j);
        // printf("Released read lock for %p\n",loc);
    }
    struct entry_with_status *ret = (struct entry_with_status *)malloc(sizeof(struct entry_with_status));
    ret->entry = entry;
    ret->status = status;
    return ret;
}

void update_cache_line(ENTRY *loc, char *key, char *val) {
    // printf("update_cache_line\n");
    
    if (loc->is_valid == 'T' && strcmp(loc->key, key) == 0)
        loc->freq ++;
    else
        loc->freq = 1;

    loc->key = strdup(key);
    loc->val = strdup(val);
    loc->is_valid = 'T';
    loc->is_dirty = 'T';
    loc->timestamp = get_microsecond_timestamp();
    // printf("Updated entry at loc %p: %s-%s(%d) \n", loc, loc->key,loc->val, loc->freq);
}

void remove_from_cache(ENTRY *loc) {
    // printf("remove_from_cache, loc = %p\n", loc);
    loc->is_valid = 'F';
    loc->freq = 0;
}

void update_frequency_timestamp(ENTRY *loc){
    loc->freq++;
    loc->timestamp = get_microsecond_timestamp();
}

#endif //CACHE_H