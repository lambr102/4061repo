#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

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
	FILE *fd = fopen(file_name,"rb"); // ok so maybe this shouldn't be all permissions but here we are
	if(fd == NULL){
		perror("fopen");
		return -1;
	}
	int size = getpagesize();
	char readin[size];
    int n_bytes;
	while ((n_bytes = fread(readin, 1 ,size, fd)) > 0){ 
		if (n_bytes == -1){
			perror("read");
			fclose(fd);
			return -1;
		}
		for (int i = 0; i < n_bytes; i++){
			int current = tolower(readin[i]);
			if (current >= 97 && current <= 122){
				counts[current - 97]++;
			}
		}
	}
	fclose(fd);
	return 0;
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
    int result = count_letters(file_name, counts);
    if(result == -1){
        close(out_fd);
    	return -1;
    }
    else if (write(out_fd, &counts, sizeof(counts)) == -1) {
    	perror("write");
        close(out_fd);
	    return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // No files to consume, return immediately
        return 0;
    }
    int fds[2];
    int child_pipe = pipe(fds);
    if (child_pipe < 0){
        perror("pipe");
        return 1;
    }
    for (int i = 1; i < argc; i ++){
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            close(fds[0]);
            close(fds[1]);
            return 1;
        } else if (child_pid == 0){
            if (close(fds[0]) == -1){
                perror("close");
                close(fds[1]);
                exit(1);
            }

            if (process_file(argv[i], fds[1]) == -1){
                close(fds[0]);
                close(fds[1]);
                exit(1);
            }

	    if(close(fds[1]) == -1){
	    	perror("close");
		    exit(1);
	    }
	    exit(0);
        }
    }
    if (close(fds[1]) == -1){
    	perror("close");
	    close(fds[0]);
	    return -1;
    }
    int counts[ALPHABET_LEN] = {0};
    int temp[ALPHABET_LEN];
    int nbytes;
    int status;
    for(int waiter = 1; waiter < argc; waiter++){
    	wait(&status);
        if (!WIFEXITED(status)){
            perror("waiit");
            close(fds[0]);
            return -1;
        }
    }

    while ((nbytes = read(fds[0], &temp, sizeof(temp))) > 0) {
    	if(nbytes == -1){
            perror("read");
            close(fds[0]);
            return -1;
	    }
        for(int index = 0; index < ALPHABET_LEN; index++){
            counts[index] = counts[index] + temp[index];
        }
    }

    if (close(fds[0]) == -1){
		perror("close");
		return -1;
    }


    // TODO Fork a child to analyze each specified file (names are argv[1], argv[2], ...)
    // TODO Aggregate all the results together by reading from the pipe in the parent

    // TODO Change this code to print out the total count of each letter (case insensitive)
    for (int i = 0; i < ALPHABET_LEN; i++) {
	    printf("%c Count: %d\n", 'a' + i, counts[i]);
    }


    return 0;
}
