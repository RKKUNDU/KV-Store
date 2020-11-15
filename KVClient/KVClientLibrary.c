#include "KVClientLibrary.h"

double total_get_time=0;
double total_put_time=0;
pthread_mutex_t get_time_lock, put_time_lock, del_time_lock;

struct time_stats *ts;

struct time_stats* initialise_timer() {
    ts = (struct time_stats *)malloc(sizeof(struct time_stats));
    ts->total_get_time = 0;
    ts->total_put_time = 0;
    ts->total_del_time = 0;
    pthread_mutex_init(&get_time_lock, NULL);
    pthread_mutex_init(&put_time_lock, NULL);
    pthread_mutex_init(&del_time_lock, NULL);
    return ts;
}

void destroy_timer(struct time_stats* ts) {
    free(ts);
}

double get_total_get_time() {
    return ts->total_get_time;
}

double get_total_put_time() {
    return ts->total_put_time;
}

double get_total_del_time() {
    return ts->total_del_time;
}

// Makes 's' a string of length KV_LEN, where the last byte (index = KV_LEN) is \0
char* add_padding(char *s) {
    char* ret = (char*) malloc(sizeof(char) * (KV_LEN + 1));
    strcpy(ret, s);

    for (int i = strlen(s); i < KV_LEN; i++) {
            ret[i] = '.';
    }

    ret[KV_LEN] = '\0';

    return ret;
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
    if (!key) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Memory not allocated to key", KV_LEN);
        return -1;
    } else if (strlen(key) > KV_LEN) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Key size is greater than 256B", KV_LEN);
        return -1;
    }
    
    char* padded_key = add_padding(key);
    char* padded_val = add_padding("\0"); // It will generate a length 256 bytes string

    // Form the request
    char req[RSIZE + 1];
    req[RSIZE] = '\0';
    req[0] = '1';
    strncpy(req + KEY_START_IDX, padded_key, KV_LEN);
    strncpy(req + VAL_START_IDX, padded_val, KV_LEN);
    
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    // Send request
    int writen = write(serverfd, req, RSIZE);

    // Wait for response
    char* resp = (char*) malloc(sizeof(char) * (RSIZE + 1));
    ssize_t readn = read(serverfd, resp, RSIZE);
    resp[RSIZE] = '\0';

    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent;
    if ((end.tv_nsec-start.tv_nsec) < 0) {
        struct timespec temp;
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = BILLION+end.tv_nsec-start.tv_nsec;
        time_spent = temp.tv_sec + temp.tv_nsec / BILLION;
    } else {
        time_spent = ((end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec)) / BILLION;
    }

    pthread_mutex_lock(&get_time_lock);
    ts->total_get_time+=time_spent;
    pthread_mutex_unlock(&get_time_lock);
    
    free(padded_key);
    free(padded_val);

    // Malloc val/error memory
    if (readn <= 0 || (unsigned char) resp[0] == ERROR) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, resp + VAL_START_IDX, KV_LEN);
        (*error)[KV_LEN] = '\0';
        remove_padding(*error);
        printf("Error: %s\n", *error);
        return -1;
    }

    *val = (char*) malloc(sizeof(char) * (KV_LEN + 1));
    strncpy(*val, resp + VAL_START_IDX, KV_LEN);
    (*val)[KV_LEN] = '\0';
    remove_padding(*val);
    return 0;
}


int put(char* key, char* val, char** error, int serverfd) {
    if(!key) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Memory not allocated to key", KV_LEN);
        return -1;
    } else if(strlen(key) > KV_LEN) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Key size is greater than 256B", KV_LEN);
        return -1;
    }

    if(!val) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Memory not allocated to val", KV_LEN);
        return -1;
    } else if(strlen(val) > KV_LEN) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Value size is greater than 256B", KV_LEN);
        return -1;
    }

    char* padded_key = add_padding(key);
    char* padded_val = add_padding(val);

    char req[RSIZE + 1];
    req[0] = '2';
    req[RSIZE] = '\0';
    strncpy(req + KEY_START_IDX, padded_key, KV_LEN);
    strncpy(req + VAL_START_IDX, padded_val, KV_LEN);

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    // Send request        
    size_t writen = write(serverfd, req, RSIZE);

    // Wait for response
    char* resp = (char*) malloc(sizeof(char) * (RSIZE + 1));
    size_t readn = read(serverfd, resp, RSIZE);
    resp[RSIZE] = '\0';

    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent;
    if ((end.tv_nsec-start.tv_nsec) < 0) {
        struct timespec temp;
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = BILLION+end.tv_nsec-start.tv_nsec;
        time_spent = temp.tv_sec + temp.tv_nsec / BILLION;
    } else {
        time_spent = ((end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec)) / BILLION;
    }
    
    pthread_mutex_lock(&put_time_lock);
    ts->total_put_time+=time_spent;
    pthread_mutex_unlock(&put_time_lock);
    
    // Malloc error memory
    free(padded_key);
    free(padded_val);

    if (readn <= 0) {
        return -1;
    }

    if ((unsigned char) resp[0] == ERROR) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, resp + VAL_START_IDX, KV_LEN);
        (*error)[KV_LEN] = '\0';
        remove_padding(*error);
        return -1;
    }

    return 0;
}


int del(char* key, char** error, int serverfd) {
    if (!key) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Memory not allocated to key", KV_LEN);
        return -1;
    } else if (strlen(key) > KV_LEN) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, "Error: Key size is greater than 256B", KV_LEN);
        return -1;
    }

    char* padded_key = add_padding(key);
    char* padded_val = add_padding("\0");

    char req[RSIZE + 1];
    req[0] = '3';
    req[RSIZE] = '\0';
    strncpy(req + KEY_START_IDX, padded_key, KV_LEN);
    strncpy(req + VAL_START_IDX, padded_val, KV_LEN);

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    // Send request
    int writen = write(serverfd, req, RSIZE);

    // Wait for response
    char* resp = (char*) malloc(sizeof(char) * (RSIZE + 1));
    int readn = read(serverfd, resp, RSIZE);
    resp[RSIZE] = '\0';

    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent;
    if ((end.tv_nsec-start.tv_nsec) < 0) {
        struct timespec temp;
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = BILLION+end.tv_nsec-start.tv_nsec;
        time_spent = temp.tv_sec + temp.tv_nsec / BILLION;
    } else {
        time_spent = ((end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec)) / BILLION;
    }

    pthread_mutex_lock(&del_time_lock);
    ts->total_del_time+=time_spent;
    pthread_mutex_unlock(&del_time_lock);

    free(padded_key);
    free(padded_val);

    // Malloc error memory
    if (readn <= 0 || (unsigned char) resp[0] == ERROR) {
        *error = (char*) malloc(sizeof(char) * (KV_LEN + 1));
        strncpy(*error, resp + VAL_START_IDX, KV_LEN);
        (*error)[KV_LEN] = '\0';
        remove_padding(*error);
        return -1;
    }

    return 0;
}


