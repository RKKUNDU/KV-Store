#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#include "DS_Utilities/ds_defs.h"
#include "Requests/req_handler.h"

int SERVER_PORT;
int NUM_WORKER_THREADS;
int* worker_epoll_fds;
int sockfd;
struct hash *table = NULL;
pthread_mutex_t mutex;

void read_config();
void* worker(void*);

void signal_handler(int sig) {
    close(sockfd);
    exit(0);
}

int main(int argc, char** argv) {
    // Register signal handler
    signal(SIGINT, signal_handler); // for ctrl-c

    // Read config file
    read_config();
    initialize_cache();
    initialise_ps();
    //TODO:Create Global Hash Table
    table = createHashTable();
    // Mutex required for inserting/deleting from Global Hash Table
    pthread_mutex_init(&mutex, NULL);
    
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Error in binding");
    }

    listen(sockfd, 5);

    /*
        Setup the data structures for each worker thread i.e. epoll instance specific to each thread
        epoll_create for n threads, add to array
    */
    worker_epoll_fds = (int*) malloc(sizeof(int) * NUM_WORKER_THREADS);
    int t_ids[NUM_WORKER_THREADS];
    pthread_t ids[NUM_WORKER_THREADS];
    for (int i = 0; i < NUM_WORKER_THREADS; ++i) {
        // Create epoll instance for each thread 
        worker_epoll_fds[i] = epoll_create1(0);
        if (worker_epoll_fds[i] == -1) {
            error("Failed to create epoll instance");
        }

        // Create and spawn worker thread
		t_ids[i] = i;
		pthread_create(&ids[i], NULL, worker, (void*) &t_ids[i]);
	
    }


    // Conitnuously accept new connectionsd
    int wt = 0;
    struct epoll_event ev;
    while (1) {
        // Accept incoming connection
        socklen_t clilen = sizeof(cli_addr);
        ev.data.fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        printf("Accepted new connection: %d, assigned to WT = %d\n", cli_addr.sin_port, wt);
        
        // Set event listener and assign to appropriate worker thread 
        ev.events = EPOLLIN;
        if (epoll_ctl(worker_epoll_fds[wt], EPOLL_CTL_ADD, ev.data.fd, &ev) < 0) {
            error("Error in epoll adding");
        }

        wt = ++wt % NUM_WORKER_THREADS;
    }



    for (int i = 0; i < NUM_WORKER_THREADS; ++i) {
        pthread_join(ids[i], NULL);
        close(worker_epoll_fds[i]);
    }
    
    return 0;
}

void* worker(void* arg) {
    int id = *((int*)arg);
    //TODO: create queue
    struct Queue *Q = createQueue();
    printf("Thread %d is ready\n", id);

    // Space for response
    char* resp = (char*) malloc(sizeof(char) * MSG_SIZE);

    // // Probe the file descriptors
    struct epoll_event events[8];
    
    while (1) {
        // //printf("New round\n");
        int nfds = epoll_wait(worker_epoll_fds[id], events, 8, 10000);

        char buff[MSG_SIZE];
        int buff_len = MSG_SIZE; 

        char* resp = (char*) malloc(sizeof(char) * MSG_SIZE);

        for (int i = 0; i < nfds; ++i) {
            memset(buff, 0, buff_len);
            ssize_t len = read(events[i].data.fd, buff, buff_len);
            if (len == 0) {
                close(events[i].data.fd);
                continue;
            }

            if (len < 0) {
                error("Read Error");
            }

            //printf("\n");
            //printf("WT = %d, MSG = %s\n", id, buff);

            if (buff[0] == '1' || buff[0] == '3') {
                handle_requests(buff, resp, id);
            } 
            else 
            { 
                char *key = substring(buff, 1, KEY_SIZE + 1); 
                char *val = substring(buff, KEY_SIZE + 1, KEY_SIZE + VAL_SIZE + 1); 
                pthread_mutex_lock(&mutex);

                // if any other thread is handling PUT request with same key, then don't handle that now
                // Add that PUT request to a queue. Handle that later
                if (!searchInHash(table, key)) //check global hash table
                   {
                    insertToHash(table, key);
                    pthread_mutex_unlock(&mutex);
                    handle_requests(buff,resp,id);
                    pthread_mutex_lock(&mutex);
                    deleteFromHash(table, key);
                   } 
                else {
                    add(Q, buff, events[i].data.fd);
                }
                pthread_mutex_unlock(&mutex);
            }
            while(!isEmpty(Q)) {
                char *key;
                struct QueueNode *item = top(Q);
                pthread_mutex_lock(&mutex);
                key = substring(item->req, 1, KEY_SIZE + 1);
                if (!searchInHash(table, key)) {
                    insertToHash(table,key);
                    pthread_mutex_unlock(&mutex);
                    handle_requests(buff, resp, id);
                    pthread_mutex_lock(&mutex);
                    deleteFromHash(table,key);
                    pthread_mutex_unlock(&mutex);
                    pop(Q);
                }
                else if (size(Q) != 1) {
                    pop(Q);
                    add(Q, item->req,item->clientFd);
                    pthread_mutex_unlock(&mutex);
                }
                       
            }
            printf("Resp: %s\n", resp);
            size_t write_len = write(events[i].data.fd, resp, MSG_SIZE);
            // handle_requests(buff); // Here request will be parsed and appropriate action will be taken
        }
        // handle the queued PUT requests before starting the next round of epoll_wait()
    }
}

void read_config() {
    // Open config file
    FILE* fptr = fopen("server_config.txt", "r");
    if (fptr == NULL) {
        error("Could not open config file");
    }

    /* 
        Read info from config file
        1. Server listening port
        2. No. of worker threads (n)
        3. Maximum entries in the cache
    */

    char buff[256];
    int buff_len = 256;
    // TODO: Handle error if required
    if (fgets(buff, buff_len, fptr) != NULL) {
        SERVER_PORT = atoi(buff);
    }

    if (fgets(buff, buff_len, fptr) != NULL) {
        NUM_WORKER_THREADS = atoi(buff);
    }

    if (fgets(buff, buff_len, fptr) != NULL) {
        CACHE_LEN = atol(buff);
    }

    fclose(fptr);
}
