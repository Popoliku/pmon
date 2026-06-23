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

char * peek_proc_string(pid_t pid, const void *org) {
    long *buf=NULL;
    long *o = (long *)org;
    for (int n=0; ; n++, o++) {
        buf=realloc(buf, (n+1)*sizeof(long));
        long res = ptrace(PTRACE_PEEKDATA, pid, o, NULL);
        memcpy(&buf[n], &res, sizeof(long)); 
        for (int i=0; i<sizeof(long); i++) {
            if ((res&0xFF)==0) return (char *)buf;
            res=res>>8;
	}
    }
    return NULL;
}

int handle_syscall(pid_t pid, int* status, struct user_regs_struct* regs, pr_array* prs) {
	ptrace(PTRACE_GETREGS, pid, NULL, regs);
	long syscall_nr = regs->orig_rax;
	switch(syscall_nr) {
		case SYS_open:
		case SYS_openat: {
			void* pr_addr = (syscall_nr == SYS_openat) ? (void*)regs->rsi : (void*)regs->rdi;
			char* fname = peek_proc_string(pid, pr_addr);
			printf("[%d] open() del fichero %s retorna ", pid, fname);
			if(fname) free(fname);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			int fd = regs->rax;
			printf("%d\n", fd);
			return 1;
		}
		case SYS_write: {
			size_t count = regs->rdx;
			void *pr_addr = (void *)regs->rsi;
			char buf[count];
			peek_proc_mem(pid, pr_addr, buf, count);
			printf("[%d] write() con argumentos: descritor de fichero %d, tamaño %ld, datos=\"", pid, (int)regs->rdi, count);
			fwrite(buf, count, 1, stdout);
			printf("\"\n");
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			count = regs->rax;
			printf("Retorna: %ld\n", count);
			return 1;
		}
		case SYS_read: {
		       	printf("[%d] read() con argumentos: descriptor de fichero %d, tamaño %llu\n", pid, (int)regs->rdi, regs->rdx);
		       	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
		       	ptrace(PTRACE_GETREGS, pid, NULL, regs);
		       	size_t count = regs->rax;
		       	char buf[count];
		       	void *pr_addr = (void *)regs->rsi;
		       	peek_proc_mem(pid, pr_addr, buf, count);
		       	printf("Retorna %ld, datos=\"", count);
		       	fwrite(buf, count, 1, stdout);
		      	printf("\"\n");
		       	return 1;
	       	}
		case SYS_execve: {
		 	void* pr_addr = (void*)regs->rdi;
			char* fname = peek_proc_string(pid, pr_addr);
			printf("[%d] exec() del programa %s\n", pid, fname);
			if(fname) free(fname);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		      	waitpid(pid, status, 0);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		      	waitpid(pid, status, 0);
			return 1;
		}
		case SYS_dup: {
		      	int oldfd = regs->rdi;
		      	printf("[%d] dup() con argumentos: oldfd=%d\n", pid, oldfd);
		      	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		      	waitpid(pid, status, 0);
		      	ptrace(PTRACE_GETREGS, pid, NULL, regs);
		      	printf("Retorna %lld\n", regs->rax);
		      	return 1;
	      	}
		case SYS_dup2: {
			int oldfd = regs->rdi;
			int newfd = regs->rsi;
			printf("[%d] dup2() con argumentos: oldfd=%d, newfd=%d\n", pid, oldfd, newfd);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			printf("Retorna %lld\n", regs->rax);
			return 1;
		}
		case SYS_pipe:
		case SYS_pipe2: {
			void* pr_addr = (void*) regs->rdi;
		       	printf("[%d] pipe() con argumentos: buffer con direccion %p\n", pid, pr_addr);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			long ret = regs->rax;
			if(ret == 0) {
				int fd[2];
				peek_proc_mem(pid, pr_addr, fd, sizeof(fd));
				printf("Descriptores creados: fd lectura %d, fd escritura %d\n", fd[0], fd[1]); 
			}
			printf("Retorna %ld\n", ret);
		       	return 1;
	       	}
		case SYS_clone: {
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			int event = *status >> 16;
			if(event == PTRACE_EVENT_FORK) {
				pid_t new_pid;
				ptrace(PTRACE_GETEVENTMSG, pid, NULL, &new_pid);
				add_pr(prs, new_pid, pid);
				printf("[%d] fork() = %d\n", pid, new_pid);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				waitpid(pid, status, 0);
			}
			return 1;
		}
		case SYS_wait4: {
			printf("[%d] wait()\n", pid);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			return 1;
		}
		default: 
			printf("[%d] syscall no de interes: %d\n", pid, syscall_nr);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			if(syscall_nr != SYS_exit_group) waitpid(pid, status, 0);
			return 0;
	}
}
