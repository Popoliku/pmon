#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/wait.h>

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

int handle_syscall(pid_t pid, int* status, struct user_regs_struct* regs) {
	ptrace(PTRACE_GETREGS, pid, NULL, regs);
	long syscall_nr = regs->orig_rax;
	switch(syscall_nr) {
		case SYS_open:
		case SYS_openat: {
			void* pr_addr = (syscall_nr == SYS_openat) ? (void*)regs->rsi : (void*)regs->rdi;
			char* fname = peek_proc_string(pid, pr_addr);
			printf("OPEN: fichero %s\n", fname);
			if(fname) free(fname);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			int fd = regs->rax;
			printf("FIN OPEN: retorna %d\n", fd);
			return 1;
		}
		case SYS_write: {
			size_t count = regs->rdx;
			void *pr_addr = (void *)regs->rsi;
			char buf[count];
			peek_proc_mem(pid, pr_addr, buf, count);
			printf("WRITE: descritor de fichero %d, tamaño %ld, datos=\"", (int)regs->rdi, count);
			fwrite(buf, count, 1, stdout);
			printf("\"\n");
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			count = regs->rax;
			printf("FIN WRITE: retorna %ld\n", count);
			return 1;
		}
		case SYS_read: {
		       	printf("READ: descriptor de fichero %d, tamaño %llu\n", (int)regs->rdi, regs->rdx);
		       	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
		       	ptrace(PTRACE_GETREGS, pid, NULL, regs);
		       	size_t count = regs->rax;
		       	char buf[count];
		       	void *pr_addr = (void *)regs->rsi;
		       	peek_proc_mem(pid, pr_addr, buf, count);
		       	printf("FIN READ: retorna %ld, datos=\"", count);
		       	fwrite(buf, count, 1, stdout);
		      	printf("\"\n");
		       	return 1;
	       	}
		case SYS_execve: {
		 	void* pr_addr = (void*)regs->rdi;
			char* fname = peek_proc_string(pid, pr_addr);
			printf("EXEC: programa %s\n", fname);
			if(fname) free(fname);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		      	waitpid(pid, status, 0);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		      	waitpid(pid, status, 0);
			return 1;
		}
		case SYS_dup: {
		      	int oldfd = regs->rdi;
		      	printf("DUP: oldfd=%d", oldfd);
		      	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		      	waitpid(pid, status, 0);
		      	ptrace(PTRACE_GETREGS, pid, NULL, regs);
		      	printf(" = %lld\n", regs->rax);
		      	return 1;
	      	}
		case SYS_dup2: {
			int oldfd = regs->rdi;
			int newfd = regs->rsi;
			printf("DUP2: oldfd=%d, newfd=%d", oldfd, newfd);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			printf(" = %lld\n", regs->rax);
			return 1;
		}
		case SYS_pipe:
		case SYS_pipe2: {
			void* pr_addr = (void*) regs->rdi;
		       	printf("PIPE: ");
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			ptrace(PTRACE_GETREGS, pid, NULL, regs);
			long ret = regs->rax;
			if(ret == 0) {
				int fd[2];
				peek_proc_mem(pid, pr_addr, fd, sizeof(fd));
				printf("fd lectura %d, fd escritura %d ", fd[0], fd[1]); 
			}
			printf("= %ld\n", ret);
		       	return 1;
	       	}
		case SYS_clone: {
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			int event = *status >> 16;
			if(event == PTRACE_EVENT_FORK) {
				pid_t new_pid;
				ptrace(PTRACE_GETEVENTMSG, pid, NULL, &new_pid);
				printf("[%d] fork() = %d\n", pid, new_pid);
			}
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			return 1;
		}
		case SYS_wait4: {
			printf("WAIT\n");
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			waitpid(pid, status, 0);
			return 1;
		}
		default: 
			printf("Syscall no de interes: %d\n", syscall_nr);
			ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			if(syscall_nr != SYS_exit_group) wait(NULL);
			return 0;
	}
}
