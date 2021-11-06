#include <unistd.h>
#include <stdio.h>
#include <wait.h>
#include <malloc.h>

#include "ipc.h"
#include "common.h"
#include "pa1.h"

struct {
    int i; // current process's number
    int N; // number of child processes
} self_structure;

int send_multicast(void * self, const Message * msg) {
    // self should be the data structure that has current i and N
    // void * to data structure
        write(fd1[1], &state_message2, sizeof(MessageType));
}

int main() {
    FILE *f;
    f = fopen("x.log", "a+"); // a+ (create + append) option will allow appending which is useful in a log file
    int N;
    printf("Input N from 1 to 10: ");
    scanf("%d", &N);
    self_structure.N = N;
    int *fd1[N], *fd2[N];
    pid_t *p[N];

    /*
    should be fd['from']['to'][0/1] !!!

    */
    for (int i = 0; i < N; i++) {
        fd1[i] = (int *) malloc(2 * sizeof(int));
        fd2[i] = (int *) malloc(2 * sizeof(int));
        pipe(fd1[i]);
        pipe(fd2[i]);
        close(fd1[i][1]); // fd[0] - read - fd1
        close(fd2[i][0]); // fd[1] - write - fd2
    }
    /*
     * We create the pipe before forking the processes, so they share the pipe fds.
     * If we swap these two lines, both processes create their own pipe,
     * and they have different values, thus they can't communicate using fd!
     */
    for (int i = 0; i < N; i++)
        p[i] = fork();

    for (int i = 0; i <= N; i++)
        if (p[i] == 0) {
        //in child process p
        //getppid() - returns pid of the parent of child
        self_structure.i = i;

        send_multicast

        MessageType state_message1;
        MessageType state_message2 = STARTED;
        close(fd1[0]);
        close(fd2[1]);

        fprintf(f, "child's state = STARTED\n");
        write(fd1[1], &state_message2, sizeof(MessageType));

        read(fd2[0], &state_message1, sizeof(MessageType));
        if (state_message1 == 0)
            fprintf(f, "child got parent's state = STARTED\n");

        close(fd1[1]);
        close(fd2[0]);
            break;
    } else if (i == N) {
        //in parent process
        MessageType state_message2;
        MessageType state_message1 = STARTED;
        close(fd1[1]);
        close(fd2[0]);

        fprintf(f, "parent's state = STARTED\n");
        write(fd2[1], &state_message1, sizeof(MessageType));

        read(fd1[0], &state_message2, sizeof(MessageType));
        if (state_message2 == 0)
            fprintf(f, "parent got child's state = STARTED\n");

        close(fd1[0]);
        close(fd2[1]);
        wait(NULL);
        /*
         * wait for state changes in a child of the calling process
         * state change is considered to be: the child terminated; the child was stopped by a signal;
         * or the child was resumed by a signal.S
         * if a wait is not performed, then the terminated child remains in a "zombie" state
         */
    }

    fclose(f);
    return 0;
}