#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../KVClient/KVClientLibrary.h"

#define MSG_SIZE 513
#define KEY_SIZE 256
#define VAL_SIZE 256

int SERVER_PORT;
char *SERVER_IP;
int NUM_OF_GET;
int NUM_OF_PUT;
int NUM_OF_DEL;
int NUM_OF_CLIENTS;
int NUM_OF_INTIAL_ENTRIES;

void connect_send();
void read_config();
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char** argv) {
    // Read the config file
    read_config();

    connect_send();
           
    return 0;
}

void connect_send() {
    int sockfd, n;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
        
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
    
    char* buff; 
    while (1) {
        char key[KEY_SIZE], val[VAL_SIZE], *error;
        char buff[MSG_SIZE + 1];
        int status;
        printf("Enter op status, Key & val\n");
        scanf("%d %s %s", &status, key, val);
        
        sprintf(buff, "%c%s%s", '0' + status, key, val);
        buff[MSG_SIZE] = '\0';
        if (status == 1)
        {
            get(key, &val, &error, sockfd);
        }
        else if (status == 2)
        {
            put(key, val, &error, sockfd);
        }
        else if(status == 3)
        {
            del(key, &error, sockfd);
        }
        else
        {
            put(key, val, &error, sockfd);
        }
        
        
        // n = write(sockfd, buff, MSG_SIZE);
        printf("Sent Msg: %s\n", buff);
        char* resp = (char*) malloc(sizeof(char) * MSG_SIZE);
        int readn = read(sockfd, resp, MSG_SIZE);
        resp[MSG_SIZE]=(char)0;
        unsigned char status_code = *resp;
        printf("Recv Msg: %s with status: %d\n", resp, status_code);
        if (n < 0) {
            // error("Error writing to socket");
        }
    }

    close(sockfd);

    printf("Done...\n");
}

void read_config(){
    FILE* fptr = fopen("client_config.txt", "r");
    size_t read, len;
    char * line = NULL;
    while ((read = getline(&line, &len, fptr)) != -1) {
        char * token = strtok(line, "=");
        
        while( token != NULL ) {
            if (strcmp(token, "SERVER_PORT")==0)
            {
                token = strtok(NULL, "=");
                SERVER_PORT = atoi(token);
            }
            else if (strcmp(token,"SERVER_IP")==0)
            {
                token = strtok(NULL, "=");
                
                SERVER_IP = strdup(token);
                
            }
            else if (strcmp(token,"NO_OF_GET")==0)
            {
                token = strtok(NULL, "=");
                NUM_OF_GET=  atoi(token);
            }
            else if (strcmp(token, "NO_OF_PUT")==0)
            {
                token = strtok(NULL, "=");
                NUM_OF_PUT=  atoi(token);
            }
            else if (strcmp(token , "NO_OF_DEL")==0)
            {
                token = strtok(NULL, "=");
                NUM_OF_DEL=  atoi(token);
            }
            else if (strcmp(token , "NO_OF_CLIENTS")==0)
            {
                token = strtok(NULL, "=");
                NUM_OF_CLIENTS=  atoi(token);
            }
            else if (strcmp(token , "NO_OF_INITIAL_ENTRIES")==0)
            {
                token = strtok(NULL, "=");
                NUM_OF_INTIAL_ENTRIES=  atoi(token);
            }
            else
            {
                token = strtok(NULL, "=");
            }  
        } 
    }
    printf("Phase 1...\n");
    printf("Reading from config file...\n");
    printf("SERVER_PORT: %d\n", SERVER_PORT);
    printf("SERVER_IP: %s", SERVER_IP);
    printf("NO_OF_GETS: %d\n", NUM_OF_GET);
    printf("NO_OF_PUTS: %d\n", NUM_OF_PUT);
    printf("NO_OF_DELS: %d\n", NUM_OF_DEL);
    printf("NO_OF_CLIENTS: %d\n", NUM_OF_CLIENTS);
    printf("NO_OF_INITIAL_ENTRIES: %d\n", NUM_OF_INTIAL_ENTRIES);
    printf("\n");
    fclose(fptr);
}