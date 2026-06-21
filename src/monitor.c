#include <unistd.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include "../include/parser.h"

static void peek_proc_mem(pid_t pid, const void *org, void *dst, size_t n) {
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
static char * peek_proc_string(pid_t pid, const void *org) {
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

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Número de argumentos incorrecto\n");
		return 1;
	}
	pid_t child_pid = fork();
	if(child_pid == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);	
		raise(SIGSTOP);
		execvp(argv[1], &argv[1]);
		perror("Error en el exec");
		return 1;
	} 
	int status;
	waitpid(child_pid, &status, 0);
	printf("El pid del proceso monitorizado es %d\n", child_pid);
	ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACEFORK);
	while(1) {
		printf("pmon> ");
		fflush(stdout);
		char* comando = NULL;
		int pmon_argc = 0;
		char** pmon_argv = get_input(stdin, &comando, &pmon_argc);
		if(pmon_argc == 0) continue;
		if(strcmp(pmon_argv[0], "cont") == 0) {
			if(pmon_argc != 2) {
				printf("El uso correcto del comando es: cont <pid>\n");
			} else {
				pid_t pid = atoi(pmon_argv[1]);
				//Falta verificar que el argumento pasado sea un numero o un pid valido
				int syscall_interes = 0;
				while(!syscall_interes) {
					ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
					waitpid(pid, &status, 0);
					if(WIFEXITED(status)) {
						printf("[%d] proceso terminado con codigo %d\n", pid, WEXITSTATUS(status));
						ptrace(PTRACE_DETACH, pid, NULL, NULL);
						break;
					}
					if(WIFSIGNALED(status)) {
						printf("[%d] proceso terminado por señal %d\n", pid, WTERMSIG(status));
						break;
					}
					//Falta verificar que no este parado por una señal
					printf("%d\n", status);
					struct user_regs_struct regs;
					ptrace(PTRACE_GETREGS, pid, NULL, &regs);
					long syscall_nr = regs.orig_rax;
					//printf("%d\n",syscall_nr);
					switch(syscall_nr) {
						case SYS_open:
						case SYS_openat: {
							void* pr_addr = (syscall_nr == SYS_openat) ? (void*)regs.rsi : (void*)regs.rdi;
							char* fname = peek_proc_string(pid, pr_addr);
							printf("OPEN: fichero %s\n", fname);
							if(fname) free(fname);
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							wait(NULL);
							ptrace(PTRACE_GETREGS, pid, NULL, &regs);
							int fd = regs.rax;
							printf("FIN OPEN: retorna %d\n", fd);
							syscall_interes = 1;
							break;
						}
						case SYS_write: {
							size_t count = regs.rdx;
							void *pr_addr = (void *)regs.rsi;
							char buf[count];
							peek_proc_mem(pid, pr_addr, buf, count);
							printf("WRITE: descritor de fichero %d, tamaño %ld, datos=\"", (int)regs.rdi, count);
							fwrite(buf, count, 1, stdout);
							printf("\"\n");
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							wait(NULL);
							ptrace(PTRACE_GETREGS, pid, NULL, &regs);
							count = regs.rax;
							printf("FIN WRITE: retorna %ld\n", count);
							syscall_interes = 1;
							break;
						}
						case SYS_read: {
							printf("READ: descriptor de fichero %d, tamaño %llu\n", (int)regs.rdi, regs.rdx);
						       	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
						       	wait(NULL);
						       	ptrace(PTRACE_GETREGS, pid, NULL, &regs);
						       	size_t count = regs.rax;
						       	char buf[count];
						       	void *pr_addr = (void *)regs.rsi;
						       	peek_proc_mem(pid, pr_addr, buf, count);
						       	printf("FIN READ: retorna %ld, datos=\"", count);
						       	fwrite(buf, count, 1, stdout);
						       	printf("\"\n");
						       	syscall_interes = 1;
						       	break;
						}
						case SYS_execve: {
							void* pr_addr = (void*)regs.rdi;
							char* fname = peek_proc_string(pid, pr_addr);
							printf("EXEC: programa %s\n", fname);
							if(fname) free(fname);
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							wait(NULL);
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							wait(NULL);
							syscall_interes = 1;
							break;
						}
						case SYS_dup: {
							int oldfd = regs.rdi;
							printf("DUP: oldfd=%d", oldfd);
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							waitpid(pid, &status, 0);
							ptrace(PTRACE_GETREGS, pid, NULL, &regs);
							printf(" = %lld\n", regs.rax);
							syscall_interes = 1;
							break;
						}
						case SYS_dup2: {
							int oldfd = regs.rdi;
							int newfd = regs.rsi;
							printf("DUP2: oldfd=%d, newfd=%d", oldfd, newfd);
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							waitpid(pid, &status, 0);
							ptrace(PTRACE_GETREGS, pid, NULL, &regs);
							printf(" = %lld\n", regs.rax);
							syscall_interes = 1;
							break;
						}
						case SYS_pipe: {
							printf("PIPE:\n");
							syscall_interes = 1;
							break;
						}
						case SYS_clone: {
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							waitpid(pid, &status, 0);
							int event = status >> 16;
							if(event == PTRACE_EVENT_FORK) {
								pid_t new_pid;
								ptrace(PTRACE_GETEVENTMSG, pid, NULL, &new_pid);
								printf("[%d] fork() = %d\n", pid, new_pid);
							}
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							waitpid(pid, &status, 0);
							syscall_interes = 1;
							break;
						}
						
						case SYS_wait4: {
							printf("WAIT\n");
							ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
							waitpid(pid, &status, 0);
							syscall_interes = 1;
							break;
						}
						
						default: 
						       printf("Syscall no de interes: %d\n", syscall_nr);
						       ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
						       if(syscall_nr != SYS_exit_group) wait(NULL);
					}

				}
			}
		}
		free(pmon_argv);
		free(comando);
		//ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	}
}
