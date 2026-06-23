#ifndef FD_TABLE_H
#define FD_TABLE_H

#include <sys/types.h>
#include <stdio.h>

int fd_mode(pid_t pid, int fd);

void print_fds(FILE* stream, pid_t pid, int level);

#endif
