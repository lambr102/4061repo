#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ALPHABET_LEN 26

/*
 * Counts the number of occurrences of each letter (case insensitive) in a text
 * file and stores the results in an array.
 * file_name: The name of the text file in which to count letter occurrences
 * counts: An array of integers storing the number of occurrences of each letter.
 *     counts[0] is the number of 'a' or 'A' characters, counts [1] is the number
 *     of 'b' or 'B' characters, and so on.
 * Returns 0 on success or -1 on error.
 */
int count_letters(const char *file_name, int *counts) {
	int fd = open(file_name,O_RDONLY,0777) // ok so maybe this shouldn't be all permissions but here we are 
	if(fd == -1){
		perror("open"); 		 
		return -1;
	}
	const char *readin[getpagesize()] = {0};// I googled this a getpagesize is better than a macro for page table size. 
	read(fd, readin, 1);// is this right at all?



}

/*
 * Processes a particular file(counting occurrences of each letter)
 *     and writes the results to a file descriptor.
 * This function should be called in child processes.
 * file_name: The name of the file to analyze.
 * out_fd: The file descriptor to which results are written
 * Returns 0 on success or -1 on error
 */
int process_file(const char *file_name, int out_fd) {
    int counts[ALPHABET_LEN] = {0};
    int result = count_letters(file_name, &counts);
    if(result == -1){
   	//TODO error
    	return -1;
    }
    else if (write(pipe_fds[1], &counts, sizeof(int)) == -1) {
    	perror("write");
        close(pipe_fds[1]);
	return -1;
     }

    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // No files to consume, return immediately
        return 0;
    }
    // unsure it this needs adding, directly copied from code examples.
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return -1;
    }
    // again same thing here
    for (int i = 0; i < n; i++) {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            return -1;
        } else if (child_pid == 0) {
            // Close read end of pipe
            if (close(pipe_fds[0]) == -1) {
                perror("close");
                close(pipe_fds[0]);
                exit(1);
            }
	    // I think we have redundancy here. Rework to make use of process file function
            int result = process_file(file_names[i], pipe_fds[1]);
	    if(result == -1){
	    //TODO error checking
	    exit(1); // what is the number for error on exit
	    }
            if (close(pipe_fds[1]) == -1) {
                perror("close");
                exit(1);
            }

            exit(0);
        }
    }
    // Only reached by parent process
    // Close write end of pipe
    if (close(pipe_fds[1]) == -1) {
        perror("close");
        close(pipe_fds[0]);
        return -1;
    }

    int temp;
    int nbytes;
    // this should be changed to that lower for loop style. I think
    while ((nbytes = read(pipe_fds[0], &temp, sizeof(int))) > 0) {
        printf("Result: %d\n", temp);
    }
    if (nbytes == -1) {
        perror("read");
        close(pipe_fds[0]);
        return -1;
    }

    if (close(pipe_fds[0]) == -1) {
        perror("close");
        return -1;
    }
    return 0;
    

    


    // TODO Create a pipe for child processes to write their results
    // TODO Fork a child to analyze each specified file (names are argv[1], argv[2], ...)
    // TODO Aggregate all the results together by reading from the pipe in the parent

    // TODO Change this code to print out the total count of each letter (case insensitive)
    for (int i = 0; i < ALPHABET_LEN; i++) {
        printf("%c Count: %d\n", 'a' + i, -1);
    }
    return 0;
}
