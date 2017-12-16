//
// Created by kkyse on 12/12/2017.
//

#include "pipe_networking.h"
#include "util/utils.h"
#include "util/random.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#define NUM_RANDOM_LETTERS 64

#ifndef NAME_MAX
    #define NAME_MAX 255
#endif

const TwoWayPipe BAD_TWO_WAY_PIPE = {.error = -1};

static int create_in_pipe(const char *const pipe_name) {
    // only this program will read, all other programs only write
    const mode_t in_pipe_mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IWGRP;
    if (mkfifo(pipe_name, in_pipe_mode) == -1) {
        perror("mkfifo(in_pipe_name, in_pipe_mode");
        return -1;
    }
    return 0;
}

static int create_and_open_server_in_pipe(const char *const pipe_name) {
    if (create_in_pipe(pipe_name) == -1) {
        return -1;
    }
    printf("waiting for client\n");
    const int in_fd = open(pipe_name, O_RDONLY);
    if (in_fd == -1) {
        perror("open(in_pipe_name)");
        return -1;
    }
    return in_fd;
}

static int read_pipe_name(const int in_fd, char *const out_pipe_name, size_t *name_length) {
    const size_t name_max = *name_length;
    const ssize_t signed_out_pipe_name_length = read(in_fd, out_pipe_name, name_max);
    if (signed_out_pipe_name_length == -1) {
        perror("read(in_fd, out_pipe_name, name_max)");
        return -1;
    }
    const size_t out_pipe_name_length = (size_t) signed_out_pipe_name_length;
    out_pipe_name[out_pipe_name_length] = 0; // set null-terminator in case it wasn't send over the pipe
    printf("Client pipe received: \"%s\"\n", out_pipe_name);
    
    const int out_fd = open(out_pipe_name, O_WRONLY);
    if (out_fd == -1) {
        perror("open(out_pipe_name, O_WRONLY");
        return -1;
    }
    *name_length = out_pipe_name_length;
    return out_fd;
}

int unlink_if_exists(const char *const path) {
    if (unlink(path) == -1) {
        // client may have remove FIFO
        if (errno != ENOENT) {
            perror("unlink(path)");
            return -1;
        }
        errno = 0;
    }
    return 0;
}

static int client_handshake_existing_in_pipe(const int in_fd, const char *in_pipe_name) {
    char out_pipe_name[NAME_MAX + 1];
    size_t out_pipe_name_length = NAME_MAX;
    const int out_fd = read_pipe_name(in_fd, out_pipe_name, &out_pipe_name_length);
    if (out_fd == -1) {
        return -1;
    }
    
    if (write(out_fd, out_pipe_name, out_pipe_name_length) != out_pipe_name_length) {
        perror("write(out_fd, out_pipe_name, out_pipe_name_length)");
        return -1;
    }
    
    if (unlink_if_exists(out_pipe_name) == -1) {
        return -1;
    }
    if (unlink_if_exists(in_pipe_name) == -1) {
        return -1;
    }
    
    return out_fd;
}

TwoWayPipe client_handshake(const char *const in_pipe_name) {
    const int in_fd = create_and_open_server_in_pipe(in_pipe_name);
    if (in_fd == -1) {
        return BAD_TWO_WAY_PIPE;
    }
    
    const int out_fd = client_handshake_existing_in_pipe(in_fd, in_pipe_name);
    if (out_fd == -1) {
        if (close(in_fd) == -1) {
            perror("close(in_fd)");
        }
        return BAD_TWO_WAY_PIPE;
    }
    
    printf("Handshake with client finished: in = %d, out = %d\n", in_fd, out_fd);
    return (TwoWayPipe) {.in_fd = in_fd, .out_fd = out_fd};
}

static ssize_t generate_random_pipe_name_of_size(char *const pipe_name,
                                                 const size_t name_max, const size_t max_num_random_letters) {
    pipe_name[name_max] = 0; // to be safe
    const int signed_offset = snprintf(pipe_name, name_max, "%d", getpid());
    if (signed_offset == -1) {
        perror("snprintf(pipe_name, name_max, \"%d\", getpid())");
        return -1;
    }
    const size_t offset = (size_t) signed_offset;
    const size_t num_random_letters = min(max_num_random_letters, name_max - offset);
    random_lowercase_letters(pipe_name + offset, num_random_letters);
    const size_t name_length = offset + num_random_letters;
    pipe_name[name_length] = 0;
    return name_length;
}

static int open_with_timeout(const char *const path, int flags, uint seconds) {
    int fd;
    uint elapsed = 0;
    const uint interval = 1;
    do {
        fd = open(path, flags);
        if (fd != -1) {
            return fd;
        }
        sleep(interval);
        elapsed += interval;
    } while (elapsed < seconds);
    return -1;
}

typedef unsigned int uint;

static int connect_to_out_pipe(const char *const out_pipe_name) {
    const int out_fd = open_with_timeout(out_pipe_name, O_WRONLY, 3);
    if (out_fd == -1) {
        perror("open(out_pipe_name, O_WRONLY)");
        return -1;
    }
    if (unlink_if_exists(out_pipe_name) == -1) {
        return -1;
    }
    return out_fd;
}

static ssize_t create_secure_pipe(char *const pipe_name, const size_t name_max) {
    const ssize_t signed_name_length =
            generate_random_pipe_name_of_size(pipe_name, name_max, NUM_RANDOM_LETTERS);
    if (signed_name_length == -1) {
        return -1;
    }
    const size_t name_length = (size_t) signed_name_length;
    
    if (create_in_pipe(pipe_name) == -1) {
        return -1;
    }
    return name_length;
}

#define insecure_pipes() fprintf(stderr, "secure pipe names do not match")

static int verify_secure_in_pipe_name(const int in_fd,
                                      const char *const in_pipe_name, const size_t name_length) {
    char *const in_pipe_name_received = (char *) malloc(name_length * sizeof(char));
    
    if (read(in_fd, in_pipe_name_received, name_length) != name_length) {
        perror("read(in_fd, in_pipe_name_received, name_length)");
        insecure_pipes();
        return -1;
    }
    
    if (strncmp(in_pipe_name, in_pipe_name_received, name_length) != 0) {
        insecure_pipes();
        return -1;
    }
    
    printf("Client pipe verified: \"%s\"\n", in_pipe_name_received);
    
    free(in_pipe_name_received);
    return 0;
}

#undef insecure_pipes

static int securely_connect_to_client_in_pipe(const char *const in_pipe_name, const size_t name_length) {
    const int in_fd = open(in_pipe_name, O_RDONLY);
    if (in_fd == -1) {
        perror("open(in_pipe_name, O_RDONLY)");
        return -1;
    }
    
    if (verify_secure_in_pipe_name(in_fd, in_pipe_name, name_length) == -1) {
        return -1;
    }
    
    if (unlink_if_exists(in_pipe_name) == -1) {
        return -1;
    }
    
    return in_fd;
}

static int server_handshake_existing_out_pipe(const int out_fd) {
    _Static_assert(NUM_RANDOM_LETTERS < NAME_MAX, "NUM_RANDOM_LETTERS is too big, >= NAME_MAX");
    char in_pipe_name[NAME_MAX + 1];
    const ssize_t signed_name_length = create_secure_pipe(in_pipe_name, NAME_MAX);
    if (signed_name_length == -1) {
        return -1;
    }
    const size_t name_length = (size_t) signed_name_length;
    
    printf("Client pipe created: \"%s\"\n", in_pipe_name);
    
    // must send name to server before opening pipe, b/c will block on open()
    if (write(out_fd, in_pipe_name, name_length) != name_length) {
        perror("write(out_fd, in_pipe_name, name_length)");
        return -1;
    }
    
    const int in_fd = securely_connect_to_client_in_pipe(in_pipe_name, name_length);
    if (in_fd == -1) {
        return -1;
    }
    
    return in_fd;
}

TwoWayPipe server_handshake(const char *const out_pipe_name) {
    const int out_fd = connect_to_out_pipe(out_pipe_name);
    if (out_fd == -1) {
        return BAD_TWO_WAY_PIPE;
    }
    
    const int in_fd = server_handshake_existing_out_pipe(out_fd);
    if (in_fd == -1) {
        if (close(out_fd) == -1) {
            perror("close(out_fd)");
        }
        return BAD_TWO_WAY_PIPE;
    }
    
    printf("Handshake with server finished: in = %d, out = %d\n", in_fd, out_fd);
    return (TwoWayPipe) {.in_fd = in_fd, .out_fd = out_fd};
}

#undef NUM_RANDOM_LETTERS