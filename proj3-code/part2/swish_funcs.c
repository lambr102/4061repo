#include "swish_funcs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
    if (in_idx != -1){
        if (dup2(pipes[in_idx], STDIN_FILENO) == -1){
            perror("dup2");
            return -1;
        }
    }
    if (out_idx != -1){
        if (dup2(pipes[out_idx], STDIN_FILENO) == -1){
            perror("dup2");
            return -1;
        }
    }
    if (run_command(&tokens) == -1){
        printf("Error on Run Command");
        return -1;
    }
    return 0;
}

int run_pipelined_commands(strvec_t *tokens) {
    int num_pipes = strvec_num_occurrences(&tokens, "|");
    int *pipe_fds = malloc(2 * num_pipes * sizeof(int));

    int start_idx = 0;
    int end_idx = 0;


    for (int i = 0; i < num_pipes; i ++){
        strvec_t *temp_vec;
        if (strvec_init(&temp_vec) == -1){
            return -1;
        }
        for (int j = 0; strvec_get(&tokens, j) != "|"; j++ ){
            end_idx ++;
        }
        if (strvec_slice(&tokens, &temp_vec, start_idx, end_idx) == -1){
            return -1;
        }
        pid_t child = fork();
        if (child == -1){
            perror("fork");
            free(pipe_fds);
            return -1;
        } else if (child == 0){
            if (i == 0){
                run_piped_command(temp_vec, pipe_fds, num_pipes, -1, i +1);
            } else if ()
        }
    }
    return 0;
}
