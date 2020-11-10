#include "KVClientLibrary.h"

double total_get_time=0;
double total_put_time=0;
pthread_mutex_t get_time_lock, put_time_lock;

struct time_stats *ts;

struct time_stats* initialise_timer(){
    ts = (struct time_stats *)malloc(sizeof(struct time_stats));
    ts->total_get_time = 0;
    ts->total_put_time = 0;
    ts->total_del_time = 0;
    return ts;
}

void destroy_timer(struct time_stats* ts){
    free(ts);
}

double get_total_get_time(){
    return ts->total_get_time;
}

double get_total_put_time(){
    return ts->total_put_time;
}

double get_total_del_time(){
    return ts->total_del_time;
}

void add_padding(char *s) {
    if (strlen(s) >= KV_LEN)
        return ;

    for (int i = strlen(s); i < KV_LEN; i++) {
            s[i] = '.';
    }
}

void remove_padding(char *s) {
    for (int i = 0; i < KV_LEN; i++) {
        if (s[i] == '.') {
            s[i] = '\0';
            return;
        }
    }
}

int get(char* key, char** val, char** error, int serverfd) {
    if( !key ){
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Memory not allocated to key", KV_LEN);
        return -1;
    }
    else if( strlen(key) > KV_LEN)
    {
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Key size is greater than 256B", KV_LEN);
        return -1;
    }
    // Form the request
    struct timespec start, end;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    // setsockopt(serverfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    add_padding(key);
    char req[RSIZE];
    req[0] = '1';
    strncpy(req + KEY_START_IDX, key, KV_LEN);
    bzero(req + VAL_START_IDX, KV_LEN);
    
    // Send request
    clock_gettime(CLOCK_REALTIME, &start);
    int writen = write(serverfd, req, RSIZE);

    // Wait for response
    char* resp = (char*) malloc(sizeof(char) * RSIZE);
    int readn = read(serverfd, resp, RSIZE);

    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent;
    if ((end.tv_nsec-start.tv_nsec)<0)
    {
        struct timespec temp;
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = BILLION+end.tv_nsec-start.tv_nsec;
        time_spent = temp.tv_sec + temp.tv_nsec / BILLION;
    }
    else
    {
        time_spent = ((end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec)) / BILLION;
    }
    // pthread_mutex_lock(&get_time_lock);
    ts->total_get_time+=time_spent;
    // pthread_mutex_unlock(&get_time_lock);
    // printf("Response time for GET is %f micro-seconds\n", time_spent*pow(10,6));
    
    remove_padding(key);
    // Malloc val/error memory
    if (readn <= 0 || (unsigned char) resp[0] == ERROR) {
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, resp + VAL_START_IDX, KV_LEN);
        remove_padding(*error);
        return -1;
    }

    *val = (char*) malloc(sizeof(char) * KV_LEN);
    strncpy(*val, resp + VAL_START_IDX, KV_LEN);
    remove_padding(*val);
    return 0;
}


int put(char* key, char* val, char** error, int serverfd) {
    if( !key ){
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Memory not allocated to key", KV_LEN);
        return -1;
    }
    else if( strlen(key) > KV_LEN)
    {
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Key size is greater than 256B", KV_LEN);
        return -1;
    }
    if( !val ){
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Memory not allocated to val", KV_LEN);
        return -1;
    }
    else if( strlen(val) > KV_LEN)
    {
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Value size is greater than 256B", KV_LEN);
        return -1;
    }
    char req[RSIZE];
    req[0] = '2';
    
    add_padding(key);
    add_padding(val);
    // printf("After padding: key %ld val %ld\n", strlen(key), strlen(val));
    struct timespec start, end;
    // struct timeval tv;
    // tv.tv_sec = 5;
    // tv.tv_usec = 0;
    // setsockopt(serverfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    strncpy(req + KEY_START_IDX, key, KV_LEN);
    strncpy(req + VAL_START_IDX, val, KV_LEN);

    // Send request
    clock_gettime(CLOCK_REALTIME, &start);
    size_t writen = write(serverfd, req, RSIZE);

    // Wait for response
    char* resp = (char*) malloc(sizeof(char) * RSIZE);
    size_t readn = read(serverfd, resp, RSIZE);
    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent;
    if ((end.tv_nsec-start.tv_nsec)<0)
    {
        struct timespec temp;
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = BILLION+end.tv_nsec-start.tv_nsec;
        time_spent = temp.tv_sec + temp.tv_nsec / BILLION;
    }
    else
    {
        time_spent = ((end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec)) / BILLION;
    }
    
    // pthread_mutex_lock(&put_time_lock);
    ts->total_put_time+=time_spent;
    // pthread_mutex_unlock(&put_time_lock);
	// printf("Response time for PUT is %f micro-seconds \n", time_spent*pow(10,6));
    
    remove_padding(key);
    remove_padding(val);
    // printf("After remove_padding %ld %ld %s\n", strlen(key), strlen(val), resp);
    // Malloc error memory
    if (readn <= 0) {
        return -1;
    }

    if ((unsigned char) resp[0] == ERROR) {
        // printf("Error: %s\n", *error);
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, resp + VAL_START_IDX, KV_LEN);
        remove_padding(*error);
        return -1;
    }
    return 0;
}


int del(char* key, char** error, int serverfd) {
    if( !key ){
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Memory not allocated to key", KV_LEN);
        return -1;
    }
    else if( strlen(key) > KV_LEN)
    {
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, "Error: Key size is greater than 256B", KV_LEN);
        return -1;
    }
    char req[RSIZE];
    struct timespec start, end;
    req[0] = '3';
    add_padding(key);
    strncpy(req + KEY_START_IDX, key, KV_LEN);

    // Send request
    clock_gettime(CLOCK_REALTIME, &start);
    int writen = write(serverfd, req, RSIZE);

    // Wait for response
    char* resp = (char*) malloc(sizeof(char) * RSIZE);
    int readn = read(serverfd, resp, RSIZE);
    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent;
    if ((end.tv_nsec-start.tv_nsec)<0)
    {
        struct timespec temp;
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = BILLION+end.tv_nsec-start.tv_nsec;
        time_spent = temp.tv_sec + temp.tv_nsec / BILLION;
    }
    else
    {
        time_spent = ((end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec)) / BILLION;
    }
    ts->total_del_time+=time_spent;

    remove_padding(key);
    // Malloc error memory
    if (readn <= 0 || (unsigned char) resp[0] == ERROR) {
        *error = (char*) malloc(sizeof(char) * KV_LEN);
        strncpy(*error, resp + VAL_START_IDX, KV_LEN);
        remove_padding(*error);
        return -1;
    }

    return 0;
}


