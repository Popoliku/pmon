#ifndef SYSCALL_HANDLER_H
#define SYSCALL_HANDLER_H

#include <sys/types.h>
#include "pr_tree.h"

void peek_proc_mem(pid_t pid, const void* org, void* dst, size_t n);
char* peek_proc_string(pid_t pid, const void* org);

void handle_syscall_entrada(pid_t pid, pr_array* prs);
void handle_syscall_salida(pid_t pid, int* status, pr_array* prs);

#endif
