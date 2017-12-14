//
// Created by kkyse on 12/12/2017.
//

#ifndef SYSTEMS_PIPE_NETWORKING_H
#define SYSTEMS_PIPE_NETWORKING_H

#define SERVER_PIPE "server_pipe"

#include <stdint.h>
#include <stdbool.h>

typedef union two_way_pipe {
    const int64_t error;
    const int32_t fds[2];
    struct {
        const int32_t in_fd;
        const int32_t out_fd;
    };
} TwoWayPipe;

extern const TwoWayPipe BAD_TWO_WAY_PIPE;

/**
 * Handshake with the client
 * by creating and opening the in pipe to the server,
 * reading the named pipe name sent by the client,
 * opening it and returning the file descriptor,
 * and sending the name back to the client to verify the integrity of the connection.
 * Both named pipes, in and out, will be unlinked if they still exist
 * (the client may unlink them first).
 *
 * @param [in] in_pipe_name the name of the in pipe to the server (not yet created)
 * @return the file descriptors of the in and out pipes to and from the server in a TwoWayPipe
 *         TwoWayPipe.in_fd is the in pipe file descriptor
 *         TwoWayPipe.out_fd is the out pipe file descriptor
 *         if error, TwoWayPipe.value = -1
 */
TwoWayPipe client_handshake(const char *in_pipe_name);

/**
 * TODO
 *
 * @param [in] out_pipe_name the name of the out pipe to the server (not yet created)
 * @return the file descriptors of the in and out pipes to and from the client in a TwoWayPipe
 *         TwoWayPipe.in_fd is the in pipe file descriptor
 *         TwoWayPipe.out_fd is the out pipe file descriptor
 *         if error, TwoWayPipe.value = -1
 */
TwoWayPipe server_handshake(const char *out_pipe_name);

#endif // SYSTEMS_PIPE_NETWORKING_H
