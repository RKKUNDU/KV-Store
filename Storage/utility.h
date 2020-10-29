#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MSG_SIZE 9
#define KEY_SIZE 4
#define VAL_SIZE 4

char *substring(char *str, int start, int end);
void error (char* msg);

// Utility function to find substring of str
char *substring(char *str, int start, int end) {
    int bytes = (end - start + 1);
    char *substr = (char *)malloc(bytes);

    for (int i = start; i < end; i++) {
        substr[i - start] = str[i];
    }

    return substr;
}

// Utility function to print error message and exit
void error (char* msg) {
    perror(msg);
    exit(1);
}