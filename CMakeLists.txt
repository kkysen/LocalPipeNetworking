cmake_minimum_required(VERSION 3.9)
project(LocalPipeNetworking)

set(CMAKE_CXX_STANDARD 11)

add_executable(Server
        util/io.h
        util/random.c
        util/random.h
        util/sigaction.h
        util/stacktrace.c
        util/stacktrace.h
        util/string_builder.c
        util/string_builder.h
        util/string_utils.c
        util/string_utils.h
        util/utils.c
        util/utils.h
        basic_server.c
        basic_server.h
        pipe_networking.c
        pipe_networking.h)

add_executable(Client
        util/io.h
        util/random.c
        util/random.h
        util/sigaction.h
        util/stacktrace.c
        util/stacktrace.h
        util/string_builder.c
        util/string_builder.h
        util/string_utils.c
        util/string_utils.h
        util/utils.c
        util/utils.h
        client.c
        client.h
        pipe_networking.c
        pipe_networking.h)