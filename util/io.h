//
// Created by kkyse on 12/14/2017.
//

#ifndef SYSTEMS_IO_H
#define SYSTEMS_IO_H

//#define _read_or_write(read_or_write, fd, buf, size, error_label)
//    do {
//        const ssize_t _length = read_or_write(fd, buf, size);
//        if (_length == -1) {
//            perror(#read_or_write"("#fd", "#buf", "#size")");
//            goto error_label;
//        }
//    } while (false)
//
//#define read(fd, buf, size, error_label) _read_or_write(read, fd, buf, size, error_label)
//
//#define write(fd, buf, size, error_label) _read_or_write(write, fd, buf, size, error_label)

#define close(fd) if (close(fd) == -1) perror("close("#fd")");

#endif //SYSTEMS_IO_H
