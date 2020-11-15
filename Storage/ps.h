#include "utility.h"
#include "../RW_lock/rwlock.h"
#include <errno.h>
#define NUM_OF_FILES 67
#define INDEXER_LEN 997
#define ENTRY_SIZE (KEY_SIZE + VAL_SIZE + 1) // KEY_SIZE + VAL_SIZE + 1

pthread_mutex_t file_lock[NUM_OF_FILES];
extern int errno;
struct indexer{
    int cnt;
    struct indexer_node *head;
    pthread_mutex_t mutex;
};

struct indexer_node {
    unsigned long long digest;
    unsigned int file_entry_no;
    struct indexer_node *next;
};

struct indexer indexer_array[INDEXER_LEN];

char *get_file_name(char *filename, int number)
{
    filename[0] =(char)0;
    char file[30] ="Persistent_Store/data";
    char num[3];
    sprintf(num, "%d", number);
    strcat(file, num);
    strcat(file, ".txt");
    strcat(filename, file);
    return filename;
}

void initialise_ps()
{
    for (int i = 0; i < NUM_OF_FILES; i++) {
        char filename[30];
        get_file_name(filename, i);
        pthread_mutex_init(&(file_lock[i]), NULL);

        int fp = open(filename, O_TRUNC | O_CREAT, 0666);
        if( fp == -1 ) {
            fprintf(stderr, "Value of errno: %d\n", errno);
            fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        } else {
            close(fp);
        }
        close(fp);
    }

    for (int i = 0; i < INDEXER_LEN; i++) {
        indexer_array[i].cnt = 0;
        indexer_array[i].head = NULL;
        pthread_mutex_init(&(indexer_array[i].mutex), NULL);
    }
}

unsigned long long get_digest(const char *str) {       
    const unsigned long long mulp = 2654435789;
    unsigned long long mix = 0;
    mix ^= 104395301;

    while(*str)
        mix += (*str++ * mulp) ^ (mix >> 23);

    return mix ^ (mix << 37);
}

int get_indexer_index(char *key) {
    unsigned char *str = (unsigned char *)key;
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % INDEXER_LEN;
}

int get_file_hash_index(char *st) {
    unsigned char *str = (unsigned char *)st;
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % NUM_OF_FILES;
}

void update_PS(char *key, char *val) {
    char filename[30];
    int file_no = get_file_hash_index(key);
    get_file_name(filename, file_no);
    int indexer_index = get_indexer_index(key);
    unsigned long long digest = get_digest(key);

    int present = 0;
    FILE *fp = fopen(filename, "r+"); 
    pthread_mutex_lock(&(indexer_array[indexer_index].mutex));
    struct indexer_node *head = indexer_array[indexer_index].head;
    struct indexer_node *prev = NULL;
    char buff[ENTRY_SIZE + 1];
    buff[ENTRY_SIZE] = '\0';
    char *file_val;
    while (head) {
        if (head->digest == digest) {
            // we only store entry no. So multiply with 513 to get the exact location
            pthread_mutex_lock(&(file_lock[file_no]));
            fseek(fp, head->file_entry_no * ENTRY_SIZE , SEEK_SET); 
            fread(buff, ENTRY_SIZE, 1, fp);
            pthread_mutex_unlock(&(file_lock[file_no]));
            if (buff[0] == 'I') {
                char *file_key = substring(buff, 1, KEY_SIZE + 1); // 1......(KEY_SIZE)
                if (strcmp(file_key, key) == 0) {
                    present = 1;
                    break;
                }
            }
        }

        prev = head;
        head = head->next;
    }

    if(present) {
        sprintf(buff, "%c%s%s", 'I', key, val);
        pthread_mutex_lock(&(file_lock[file_no]));
        fseek(fp, head->file_entry_no * ENTRY_SIZE, SEEK_SET);
        fwrite(buff, ENTRY_SIZE, 1, fp);
        pthread_mutex_unlock(&(file_lock[file_no]));
    } else {
        struct indexer_node *node =  (struct indexer_node *)malloc(sizeof(struct indexer_node));
        sprintf(buff, "%c%s%s", 'I', key, val);
        pthread_mutex_lock(&(file_lock[file_no]));
        fseek(fp, 0, SEEK_END);
        node->file_entry_no = ftell(fp) / ENTRY_SIZE;
        fwrite(buff, ENTRY_SIZE, 1, fp);
        pthread_mutex_unlock(&(file_lock[file_no]));
        node->digest = digest;
        indexer_array[indexer_index].cnt++;
        if (!prev) {
            indexer_array[indexer_index].head = node;
            node->next = NULL;
        } else {
            prev->next = node;
            node->next = NULL;
        }
    }

    pthread_mutex_unlock(&(indexer_array[indexer_index].mutex));
    fclose(fp);
}

char *find_in_PS(char *key){
    char filename[30];
    int file_no = get_file_hash_index(key);
    get_file_name(filename, file_no);
    int indexer_index = get_indexer_index(key);
    unsigned long long digest = get_digest(key);

    int present = 0;
    FILE *fp = fopen (filename, "r+"); 
    pthread_mutex_lock(&(indexer_array[indexer_index].mutex));
    struct indexer_node *head = indexer_array[indexer_index].head;
    char buff[ENTRY_SIZE + 1];
    buff[ENTRY_SIZE] = '\0';
    char *file_val;
    while (head) {
        if (head->digest == digest) {
            // we only store entry no. So multiply with 513 to get the exact location
            pthread_mutex_lock(&(file_lock[file_no]));
            fseek(fp, head->file_entry_no * ENTRY_SIZE , SEEK_SET); 
            fread(buff, ENTRY_SIZE, 1, fp);
            pthread_mutex_unlock(&(file_lock[file_no]));
            if (buff[0] == 'I') {
                char *file_key = substring(buff, 1, KEY_SIZE + 1); // 1......(KEY_SIZE)
                if (strcmp(file_key, key) == 0) {
                    present = 1;
                    file_val = substring(buff, KEY_SIZE + 1, KEY_SIZE + VAL_SIZE + 1); // (KEY_SIZE + 1)......(KEY_SIZE + VAL_SIZE)
                    break;
                }
            }
        }

        head = head->next;
    }

    pthread_mutex_unlock(&(indexer_array[indexer_index].mutex));
    fclose(fp);

    if(present) {
        return file_val;
    } else {
        return NULL;
    }

}

int remove_from_PS(char *key){
    char filename[30];
    int file_no = get_file_hash_index(key);
    get_file_name(filename, file_no);
    int indexer_index = get_indexer_index(key);
    unsigned long long digest = get_digest(key);

    int present = 0;
    FILE *fp = fopen (filename, "r+"); 
    pthread_mutex_lock(&(indexer_array[indexer_index].mutex));
    struct indexer_node *head = indexer_array[indexer_index].head;
    struct indexer_node *prev = NULL;
    char buff[ENTRY_SIZE + 1];
    buff[ENTRY_SIZE] = '\0';
    char *file_val;
    while (head) {
        if (head->digest == digest) {
            // we only store entry no. So multiply with 513 to get the exact location
            pthread_mutex_lock(&(file_lock[file_no]));
            fseek(fp, head->file_entry_no * ENTRY_SIZE , SEEK_SET); 
            fread(buff, ENTRY_SIZE, 1, fp);
            pthread_mutex_unlock(&(file_lock[file_no]));
            if (buff[0] == 'I') {
                char *file_key = substring(buff, 1, KEY_SIZE + 1); // 1......(KEY_SIZE)
                if (strcmp(file_key, key) == 0) {
                    if (prev == NULL) 
                        indexer_array[indexer_index].head = head->next;
                    else 
                        prev->next = head->next;
                    
                    // write to file with 'D' entry
                    // fseek(fp, head->file_entry_no * ENTRY_SIZE , SEEK_SET); 
                    // fwrite();
                    present = 1;
                    file_val = substring(buff, KEY_SIZE + 1, KEY_SIZE + VAL_SIZE + 1); // (KEY_SIZE + 1)......(KEY_SIZE + VAL_SIZE)
                    break;
                }
            }
        }

        prev = head;
        head = head->next;
    }

    pthread_mutex_unlock(&(indexer_array[indexer_index].mutex));
    fclose(fp);
    if(present) {
        return 1;
    } else {
        return 0;
    }
}