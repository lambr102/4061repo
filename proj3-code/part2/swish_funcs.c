#include "swish_funcs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline. You should make
 * make use of the provided 'run_command' function here.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipes: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipes, int in_idx, int out_idx) {
    if (in_idx != -1) {
        if (dup2(pipes[2 * in_idx], STDIN_FILENO) == -1) {
            perror("dup2");
            return -1;
        }
    }
    if (out_idx != -1) {
        if (dup2(pipes[2 * out_idx + 1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return -1;
        }
    }
    for (int i = 0; i < 2 * n_pipes; i++) {
        if (i != in_idx || i != out_idx) {
            close(pipes[i]);
        }
    }
    if (run_command(tokens) == -1) {
        perror("run_command");
        return -1;
    }
    return 0;
}

int run_pipelined_commands(strvec_t *tokens) {
    int num_pipes = strvec_num_occurrences(tokens, "|");
    int *pipe_fds = malloc(2 * num_pipes * sizeof(int));

    if (!pipe_fds) {
        perror("malloc");
        return -1;
    }

    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_fds + 2 * i) == -1) {
            perror("pipe");
            for (int j = 0; j < i; j++) {
                close(pipe_fds[2 * j]);
                close(pipe_fds[2 * j + 1]);
            }
            free(pipe_fds);
            return -1;
        }
    }

    int num_cmds = num_pipes + 1;
    pid_t *pids = malloc(num_cmds * sizeof(pid_t));
    if (!pids) {
        perror("malloc");
        for (int i = 0; i < 2 * num_pipes; i++)
            close(pipe_fds[i]);
        free(pipe_fds);
        return -1;
    }

    int start_idx = 0;
    for (int cmd = 0; cmd < num_cmds; cmd++) {
        int in_pipe;
        int out_pipe;

        if (cmd == 0){
            in_pipe = -1;
        } else {
            in_pipe = cmd -1;
        }

        if (cmd == num_cmds - 1){
            out_pipe = -1;
        } else {
            out_pipe = cmd;
        }

        int end_idx = -1;
        for (int j = start_idx; j < (int) tokens->length; j++) {
            char *tok = strvec_get(tokens, j);
            if (tok && strcmp(tok, "|") == 0) {
                end_idx = j;
                break;
            }
        }
        if (end_idx == -1) {
            end_idx = tokens->length;
        }

        strvec_t temp_vec;
        if (strvec_slice(tokens, &temp_vec, start_idx, end_idx) == -1) {
            for (int i = 0; i < 2 * num_pipes; i++)
                close(pipe_fds[i]);
            free(pipe_fds);
            free(pids);
            return -1;
        }

        pid_t child = fork();
        if (child == -1) {
            perror("fork");
            strvec_clear(&temp_vec);
            for (int i = 0; i < 2 * num_pipes; i++)
                close(pipe_fds[i]);
            free(pipe_fds);
            free(pids);
            return -1;
        } else if (child == 0) {
            if (run_piped_command(&temp_vec, pipe_fds, num_pipes, in_pipe, out_pipe) == -1) {
                printf("Error on piped_commands");
                free(pids);
                free(pipe_fds);
                exit(1);
            }
            strvec_clear(&temp_vec);
            free(pids);
            free(pipe_fds);
            exit(0);
        }

        pids[cmd] = child;

        start_idx = end_idx + 1;
        strvec_clear(&temp_vec);

    }

    for (int i = 0; i < 2 * num_pipes; i++) {
        close(pipe_fds[i]);
    }

    free(pipe_fds);

    for (int i = 0; i < num_pipes + 1; i++) {
        if (wait(NULL) == -1){
            perror("wait");
            for (int i = 0; i < 2 * num_pipes; i++) {
                close(pipe_fds[i]);

            }
        }
    }
    free(pids);

    return 0;
}
