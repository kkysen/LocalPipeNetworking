//
// Created by kkyse on 12/12/2017.
//

#include "client.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pipe_networking.h"
#include "util/stacktrace.h"
#include "util/io.h"

int run() {
    const TwoWayPipe pipes = server_handshake(SERVER_PIPE);
    if (pipes.error == -1) {
        return -1;
    }
    
    size_t buf_size;
    char *line = NULL;
    const ssize_t signed_length = getline(&line, &buf_size, stdin);
    if (signed_length == -1) {
        goto error_close;
    }
    
    if (signed_length == 0) {
        goto error_close;
    }
    
    const size_t length = (size_t) signed_length - 1;
    if (length == 0) {
        goto error_close; // no input
    }
    
    line[length] = 0; // cutoff '\n' at end
    printf("entered: \"%s\"\n", line);
    
    {
        const ssize_t bytes_written = write(pipes.out_fd, &length, sizeof(length));
        if (bytes_written != sizeof(length)) {
            perror("write(pipes.out_fd, &length, sizeof(length))");
            if (bytes_written == -1) {
                goto error_close;
            } else {
                fprintf(stderr, "%zd != %zu", bytes_written, sizeof(length));
            }
        }
    }
    
    {
        const ssize_t bytes_written = write(pipes.out_fd, line, length);
        if (bytes_written != length) {
            perror("write(pipes.out_fd, line, length)");
            if (bytes_written == -1) {
                goto error_close;
            } else {
                fprintf(stderr, "%zd != %zu", bytes_written, length);
            }
        }
    }
    
    size_t received_length;
    {
        const ssize_t bytes_read = read(pipes.in_fd, &received_length, sizeof(received_length));
        if (bytes_read != sizeof(received_length)) {
            perror("read(pipes.in_fd, &received_length, sizeof(received_length))");
            if (bytes_read == -1) {
                goto error_close;
            } else {
                fprintf(stderr, "%zd != %zu", bytes_read, sizeof(received_length));
            }
        }
    }
    
    char *const received = (char *) calloc(received_length + 1, sizeof(char));
    if (!received) {
        perror("calloc(received_length + 1, sizeof(char))");
        goto error_close;
    }
    
    {
        const ssize_t bytes_read = read(pipes.in_fd, received, received_length);
        if (bytes_read != received_length) {
            perror("read(pipes.in_fd, received, received_length)");
            if (bytes_read == -1) {
                goto error_free;
            } else {
                fprintf(stderr, "%zd != %zu", bytes_read, received_length);
            }
        }
    }
    
    printf("Modified Text from Server: \"%s\"\n", received);
    
    free(received);
    free(line);
    close(pipes.in_fd);
    close(pipes.out_fd);
    return 0;
    
    error_free:
    free(received);
    
    error_close:
    free(line);
    close(pipes.in_fd);
    close(pipes.out_fd);
    return -1;
}

int main() {
    set_stack_trace_signal_handler();
    
    for (;;) {
        if (run() == -1) {
            return EXIT_FAILURE;
        }
    }
    
    return EXIT_SUCCESS;
}