#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include "pr_tree.h"

void peek_proc_mem(pid_t pid, const void *org, void *dst, size_t n) {
    int niter = n/sizeof(long);
    int resto = n%sizeof(long);
    long *o = (long *)org;
    long *d = (long *)dst;
    for (int i=0; i<niter; i++, o++) {
        d[i] = ptrace(PTRACE_PEEKDATA, pid, o, NULL);
    }
    if (resto) {
        long res = ptrace(PTRACE_PEEKDATA, pid, o, NULL);
        memcpy(&d[niter], &res, resto);
    }
}

char* peek_proc_string(pid_t pid, const void *org) {
    long *buf=NULL;
    long *o = (long *)org;
    for (int n=0; ; n++, o++) {
        buf=realloc(buf, (n+1)*sizeof(long));
        long res = ptrace(PTRACE_PEEKDATA, pid, o, NULL);
        memcpy(&buf[n], &res, sizeof(long));
        for (int i=0; i<(int)sizeof(long); i++) {
            if ((res&0xFF)==0) return (char *)buf;
            res=res>>8;
        }
    }
    return NULL;
}

void handle_syscall_entrada(pid_t pid, pr_array* prs) {
	int status;
	struct user_regs_struct regs;

	while (1) {
		ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			printf("[%d] proceso ha terminado con codigo %d\n", pid, WEXITSTATUS(status));
			pr_set_state(prs, pid, PR_TERMINATED);
			return;
		}
		if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGCHLD)
			continue;

		ptrace(PTRACE_GETREGS, pid, NULL, &regs);
		long syscall_nr = regs.orig_rax;

		switch (syscall_nr) {
			case SYS_open:
			case SYS_openat: {
				void* pr_addr = (syscall_nr == SYS_openat) ? (void*)regs.rsi : (void*)regs.rdi;
				char* fname = peek_proc_string(pid, pr_addr);
				printf("[%d] open() del fichero %s\n", pid, fname);
				if (fname) free(fname);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_write: {
				size_t count = regs.rdx;
				char buf[count];
				peek_proc_mem(pid, (void*)regs.rsi, buf, count);
				printf("[%d] write() con argumentos: descritor de fichero %d, tamaño %ld, datos=\"",
					pid, (int)regs.rdi, count);
				fwrite(buf, count, 1, stdout);
				printf("\"\n");
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_read: {
				printf("[%d] read() con argumentos: descriptor de fichero %d, tamaño %llu\n",
					pid, (int)regs.rdi, regs.rdx);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_execve: {
				char* fname = peek_proc_string(pid, (void*)regs.rdi);
				printf("[%d] exec() del programa %s\n", pid, fname);
				if (fname) free(fname);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				waitpid(pid, &status, 0);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				waitpid(pid, &status, 0);
				return;
			}
			case SYS_dup: {
				printf("[%d] dup() con argumentos: oldfd=%d\n", pid, (int)regs.rdi);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_dup2: {
				printf("[%d] dup2() con argumentos: oldfd=%d, newfd=%d\n",
					pid, (int)regs.rdi, (int)regs.rsi);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_pipe:
			case SYS_pipe2: {
				printf("[%d] pipe() con argumentos: buffer con direccion %p\n", pid, (void*)regs.rdi);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_socket: {
				printf("[%d] socket() argumentos: domain=%d, type=%d, protocol=%d\n",
					pid, (int)regs.rdi, (int)regs.rsi, (int)regs.rdx);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_clone: {
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				waitpid(pid, &status, 0);
				int event = status >> 16;
				if (event == PTRACE_EVENT_FORK) {
					pid_t new_pid;
					ptrace(PTRACE_GETEVENTMSG, pid, NULL, &new_pid);
					add_pr(prs, new_pid, pid);
					printf("[%d] fork() = %d\n", pid, new_pid);
					ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
					waitpid(pid, &status, 0);
				}
				return;
			}
			case SYS_wait4: {
				printf("[%d] wait()\n", pid);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			default: {
				printf("[%d] syscall no de interes: %ld\n", pid, syscall_nr);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				if (syscall_nr != SYS_exit_group)
					waitpid(pid, &status, 0);
				break;
			}
		}
	}
}

void handle_syscall_salida(pid_t pid, int* status, pr_array* prs) {
	if (WIFEXITED(*status)) {
		printf("[%d] proceso ha terminado con codigo %d\n", pid, WEXITSTATUS(*status));
		pr_set_state(prs, pid, PR_TERMINATED);
		return;
	}

	struct user_regs_struct regs;
	ptrace(PTRACE_GETREGS, pid, NULL, &regs);
	long syscall_nr = regs.orig_rax;

	switch (syscall_nr) {
		case SYS_open:
		case SYS_openat:
			printf("[%d] fin open() retorna %lld\n", pid, regs.rax);
			break;
		case SYS_write:
			printf("[%d] fin write() retorna %lld\n", pid, regs.rax);
			break;
		case SYS_read: {
			long count = (long)regs.rax;
			printf("[%d] fin read() retorna %ld", pid, count);
			if (count > 0) {
				char buf[count];
				peek_proc_mem(pid, (void*)regs.rsi, buf, count);
				printf(", datos=\"");
				fwrite(buf, count, 1, stdout);
				printf("\"");
			}
			printf("\n");
			break;
		}
		case SYS_dup:
		case SYS_dup2:
			printf("[%d] fin dup() retorna %lld\n", pid, regs.rax);
			break;
		case SYS_pipe:
		case SYS_pipe2: {
			long ret = (long)regs.rax;
			if (ret == 0) {
				int fd[2];
				peek_proc_mem(pid, (void*)regs.rdi, fd, sizeof(fd));
				printf("[%d] fin pipe() descriptores creados: fd lectura %d, fd escritura %d\n",
					pid, fd[0], fd[1]);
			}
			printf("[%d] fin pipe() retorna %ld\n", pid, ret);
			break;
		}
		case SYS_socket:
			printf("[%d] fin socket() retorna descriptor %lld\n", pid, regs.rax);
			break;
		case SYS_wait4:
			printf("[%d] fin wait() retorna %lld\n", pid, regs.rax);
			break;
		default:
			break;
	}

	pr_set_state(prs, pid, PR_STOPPED);
}
