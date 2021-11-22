#include <unistd.h>
#include <stdio.h>
#include <wait.h>
#include "ipc.h"
#include "common.h"
#include "pa1.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

struct self_structure_type {
    int id; // current process's number
    int N; // number of processes
    int ***fd;
};

int send_multicast(void *self, const Message *msg) {
    struct self_structure_type *str = (struct self_structure_type *) self;

    for (int i = 0; i < str->N; i++) {
        if (str->id == i)
            continue;
        write(
                str->fd[str->id][i][1],
                msg,
                sizeof(Message));
    }
    return 0;
}

int receive_any(void *self, Message *msg) {
    struct self_structure_type *str = (struct self_structure_type *) self;

    for (int i = 1; i < str->N; i++) {
        if (str->id == i)
            continue;

        read(
                str->fd[i][str->id][0],
                msg,
                sizeof(Message));
    }
    return 0;
}

void close_pipes(int N, int ***fd) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (fd[i][j][0] > 0) {
                close(fd[i][j][0]);
                fd[i][j][0] = -123;
            }
            if (fd[i][j][1] > 0) {
                close(fd[i][j][1]);
                fd[i][j][1] = -123;
            }
        }
    }
}

void child(FILE *events_log_file, int N, int ***fd, int i) {
    for (int q = 0; q < N; q++) {
        for (int j = 0; j < N; j++) {
            if (q != i) {
                if (fd[j][q][0] > 0) {
                    close(fd[j][q][0]);
                    fd[j][q][0] = -1;
                }
                if (fd[q][j][1] > 0) {
                    close(fd[q][j][1]);
                    fd[q][j][1] = -1;
                }
            }
        }
    }

    printf(log_started_fmt, i, getpid(), getppid()); //* пишет в лог started
    fprintf(events_log_file, log_started_fmt, i, getpid(), getppid()); //* пишет в лог started

    struct self_structure_type self_structure = {.id = (int) i, .N = (int) N, .fd = fd};

    // отправляет started всем процессам
    Message msg;
    MessageHeader s_header = {.s_magic = MESSAGE_MAGIC, .s_type = STARTED, .s_local_time = (int16_t) time(NULL)};
    s_header.s_payload_len = snprintf(msg.s_payload, MAX_PAYLOAD_LEN, log_started_fmt, i, getpid(), getppid());
    msg.s_header = s_header;
    send_multicast(&self_structure, &msg);

    // дождаться от других дочерних сообщения STARTED
    receive_any(&self_structure, &msg);
    printf(log_received_all_started_fmt, i); //* пишет в лог received all started
    fprintf(events_log_file, log_received_all_started_fmt, i); //* пишет в лог received all started

    printf(log_done_fmt, i); //* пишет в лог done
    fprintf(events_log_file, log_done_fmt, i); //* пишет в лог done

    // отправляет DONE всем
    s_header.s_type = DONE;
    s_header.s_payload_len = snprintf(msg.s_payload, MAX_PAYLOAD_LEN, log_done_fmt, i);
    s_header.s_local_time = (int16_t) time(NULL);
    msg.s_header = s_header;
    send_multicast(&self_structure, &msg);

    // дождаться от других дочерних сообщения done
    receive_any(&self_structure, &msg);
    printf(log_received_all_done_fmt, i); //* пишет в лог received all started
    fprintf(events_log_file, log_received_all_done_fmt, i); //* пишет в лог received all started

    close_pipes(N, fd);
    fclose(events_log_file);
    exit(0);
}

int main(int argc, char **argv) {
    FILE *events_log_file, *pipes_log_file;
    events_log_file = fopen(events_log,
                            "a+"); // a+ (create + append) option will allow appending which is useful in a log file
    pipes_log_file = fopen(pipes_log, "a+");
    int N = 0;
    for (int i = 0; i < argc; i++)
        if (!strcmp(argv[i], "-p")) {
            N = strtol(argv[i + 1], &argv[i + 1], 0);
            N++;
            break;
        }

    // fd['from']['to'][0/1]
    int ***fd = (int ***) malloc(N * sizeof(int **));
    for (int i = 0; i < N; i++) {
        fd[i] = (int **) malloc(N * sizeof(int *));

        for (int j = 0; j < N; j++)
            fd[i][j] = (int *) malloc(2 * sizeof(int));
    }

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            if (i != j) {
                pipe(fd[i][j]);
                fprintf(pipes_log_file, "fd[%d][%d][0] fd[%d][%d][1]\n", i, j, i, j);
            } else {
                fd[i][j][0] = -123;
                fd[i][j][1] = -123;
            }
        }
    fclose(pipes_log_file);

    /*
     * We create the pipe before forking the processes, so they share the pipe fds.
     * If we swap these two lines, both processes create their own pipe,
     * and they have different values, thus they can't communicate using fd!
     */
    //starts from 1 for less confusion because 0 is a parent
    for (int i = 1; i < N; i++) {
        if (fork() == 0) {
            child(events_log_file, N, fd, i);
        }
    }

    for (int q = 0; q < N; q++) {
        for (int j = 0; j < N; j++) {
            if (q != PARENT_ID) {
                if (fd[j][q][0] > 0) {
                    close(fd[j][q][0]);
                    fd[j][q][0] = -1;
                }
                if (fd[q][j][1] > 0) {
                    close(fd[q][j][1]);
                    fd[q][j][1] = -1;
                }
            }
        }
    }

    //in parent process
    struct self_structure_type self_structure = {.id = (int) 0, .N = (int) N, .fd = fd};
    Message msg;
    //wait started
    receive_any(&self_structure, &msg);
    //wait done
    receive_any(&self_structure, &msg);

    while (wait(NULL) > 0);

    close_pipes(N, fd);

    /*
     * wait for state changes in a child of the calling process
     * state change is considered to be: the child terminated; the child was stopped by a signal;
     * or the child was resumed by a signal.S
     * if a wait is not performed, then the terminated child remains in a "zombie" state
     */
    fclose(events_log_file);

    // deallocate memory
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            free(fd[i][j]);
        }
        free(fd[i]);
    }
    free(fd);
    return 0;
}
