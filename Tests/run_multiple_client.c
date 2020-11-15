#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2)
        return 0;

    int cnt = atoi(argv[1]);
    printf("I will fork %d clients\n", cnt);
    for (int i = 0; i < cnt; i++) {
        int pid = fork();
        sleep(0.2);
        if (pid == 0) {
            char *args[] = {"make", "run_test_client", NULL};
            execvp("make", args);
        } 
    }
}