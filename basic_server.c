//
// Created by kkyse on 12/12/2017.
//

#include "basic_server.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pipe_networking.h"
#include "util/utils.h"
#include "util/stacktrace.h"
#include "util/io.h"

void not(char *const text, const size_t length) {
    const size_t long_length = length / (sizeof(size_t) / sizeof(char));
    size_t *long_text = (size_t *) text;
    size_t *long_end = long_text + long_length;
    --long_text;
    while (++long_text < long_end) {
        *long_text = ~*long_text;
    }
    
    char *remaining_text = ((char *) long_text) - 1;
    char *end = text + length;
    while (++remaining_text < end) {
        *remaining_text = ~*remaining_text;
    }
}

void modify_text(char *const text, const size_t length) {
    not(text, length);
}

int run() {
    const TwoWayPipe pipes = client_handshake(SERVER_PIPE);
    if (pipes.error == -1) {
        return -1;
    }
    
    size_t length;
    read(pipes.in_fd, &length, sizeof(length), error_close);
    
    char *const text = (char *) malloc(length);
    if (!text) {
        perror("malloc(length)");
        goto error_close;
    }
    
    read(pipes.in_fd, text, length, error_free);
    
    modify_text(text, length);
    
    write(pipes.out_fd, &length, sizeof(length), error_free);
    write(pipes.out_fd, text, length, error_free);
    
    free(text);
    close(pipes.in_fd);
    close(pipes.out_fd);
    return 0;
    
    error_free:
    free(text);
    
    error_close:
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