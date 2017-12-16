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

#define in_range(c, start, end) ((c) >= (start) && (c) <= (end))

#define is_lowercase(c) in_range(c, 'a', 'z')

#define is_uppercase(c) in_range(c, 'A', 'Z')

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

void invert_case(char *const text, const size_t length) {
    for (size_t i = 0; i < length; ++i) {
        char c = text[i];
        if (is_lowercase(c)) {
            c += 'A' - 'a';
        } else if (is_uppercase(c)) {
            c += 'a' - 'A';
        }
        text[i] = c;
    }
}

void modify_text(char *const text, const size_t length) {
    invert_case(text, length);
}

int run() {
    const TwoWayPipe pipes = client_handshake(SERVER_PIPE);
    if (pipes.error == -1) {
        return -1;
    }
    
    size_t length;
    {
        const ssize_t bytes_read = read(pipes.in_fd, &length, sizeof(length));
        if (bytes_read != sizeof(length)) {
            if (bytes_read == -1) {
                perror("read(pipes.in_fd, &length, sizeof(length))");
                goto error_close;
            } else {
//                fprintf(stderr, "%zd != %zu", bytes_read, sizeof(length));
                length = (size_t) bytes_read;
            }
        }
    }
    
    if (length == 0 || errno == EPIPE) {
        printf("Client terminated session.\n");
        close(pipes.in_fd);
        close(pipes.out_fd);
        return 0;
    }
    
//    pz(length);
    char *const text = (char *) calloc(length + 1, sizeof(char));
    if (!text) {
        perror("calloc(length + 1, sizeof(char))");
        goto error_close;
    }
    
    {
        const ssize_t bytes_read = read(pipes.in_fd, text, length);
        if (bytes_read != length) {
            perror("read(pipes.in_fd, text, length)");
            if (bytes_read == -1) {
                goto error_free;
            } else {
                fprintf(stderr, "%zd != %zu", bytes_read, length);
            }
        }
    }
    
    printf("Received: \"%s\"\n", text);
    modify_text(text, length);
    printf("Modified: \"%s\"\n", text);
    
    {
        const ssize_t bytes_written = write(pipes.out_fd, &length, sizeof(length));
        if (bytes_written != sizeof(length)) {
            perror("write(pipes.out_fd, &length, sizeof(length))");
            if (bytes_written == -1) {
                goto error_free;
            } else {
                fprintf(stderr, "%zd != %zu", bytes_written, sizeof(length));
            }
        }
    }
    
    {
        const ssize_t bytes_written = write(pipes.out_fd, text, length);
        if (bytes_written != length) {
            perror("write(pipes.out_fd, text, length)");
            if (bytes_written == -1) {
                goto error_free;
            } else {
                fprintf(stderr, "%zd != %zu", bytes_written, length);
            }
        }
    }
    
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

void remove_server_pipe() {
    printf("removed server_pipe\n");
    unlink_if_exists(SERVER_PIPE);
}

int main() {
    set_stack_trace_signal_handler();
    
    if (atexit(remove_server_pipe) != 0) {
        perror("atexit(remove_server_pipe)");
    }
    
    for (;;) {
        if (run() == -1) {
            return EXIT_FAILURE;
        }
    }
    
    return EXIT_SUCCESS;
}