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
    
    const size_t length = (size_t) signed_length;
    write(pipes.out_fd, &length, sizeof(length), error_close);
    write(pipes.out_fd, line, length, error_close);
    
    size_t received_length;
    read(pipes.in_fd, &received_length, sizeof(received_length), error_close);
    
    char *const received = (char *) malloc(received_length);
    if (!received) {
        perror("malloc(received_length)");
        goto error_close;
    }
    
    read(pipes.in_fd, received, received_length, error_free);
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