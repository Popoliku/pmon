#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include "pr_tree.h"

char* syscall_names[335];

char* syscall_name(long nr) {
	if (!syscall_names[0]) {
		syscall_names[0]  = "read";
		syscall_names[1]  = "write";
		syscall_names[2]  = "open";
		syscall_names[3]  = "close";
		syscall_names[4]  = "stat";
		syscall_names[5]  = "fstat";
		syscall_names[6]  = "lstat";
		syscall_names[7]  = "poll";
		syscall_names[8]  = "lseek";
		syscall_names[9]  = "mmap";
		syscall_names[10] = "mprotect";
		syscall_names[11] = "munmap";
		syscall_names[12] = "brk";
		syscall_names[13] = "rt_sigaction";
		syscall_names[14] = "rt_sigprocmask";
		syscall_names[15] = "rt_sigreturn";
		syscall_names[16] = "ioctl";
		syscall_names[17] = "pread64";
		syscall_names[18] = "pwrite64";
		syscall_names[19] = "readv";
		syscall_names[20] = "writev";
		syscall_names[21] = "access";
		syscall_names[22] = "pipe";
		syscall_names[23] = "select";
		syscall_names[24] = "sched_yield";
		syscall_names[25] = "mremap";
		syscall_names[26] = "msync";
		syscall_names[28] = "madvise";
		syscall_names[32] = "dup";
		syscall_names[33] = "dup2";
		syscall_names[35] = "nanosleep";
		syscall_names[39] = "getpid";
		syscall_names[40] = "sendfile";
		syscall_names[41] = "socket";
		syscall_names[42] = "connect";
		syscall_names[43] = "accept";
		syscall_names[44] = "sendto";
		syscall_names[45] = "recvfrom";
		syscall_names[46] = "sendmsg";
		syscall_names[47] = "recvmsg";
		syscall_names[48] = "shutdown";
		syscall_names[49] = "bind";
		syscall_names[50] = "listen";
		syscall_names[56] = "clone";
		syscall_names[57] = "fork";
		syscall_names[58] = "vfork";
		syscall_names[59] = "execve";
		syscall_names[60] = "exit";
		syscall_names[61] = "wait4";
		syscall_names[62] = "kill";
		syscall_names[63] = "uname";
		syscall_names[72] = "fcntl";
		syscall_names[74] = "fsync";
		syscall_names[77] = "ftruncate";
		syscall_names[78] = "getdents";
		syscall_names[79] = "getcwd";
		syscall_names[80] = "chdir";
		syscall_names[82] = "rename";
		syscall_names[83] = "mkdir";
		syscall_names[84] = "rmdir";
		syscall_names[87] = "unlink";
		syscall_names[89] = "readlink";
		syscall_names[102] = "getuid";
		syscall_names[104] = "getgid";
		syscall_names[107] = "geteuid";
		syscall_names[108] = "getegid";
		syscall_names[110] = "getppid";
		syscall_names[158] = "arch_prctl";
		syscall_names[218] = "set_tid_address";
		syscall_names[231] = "exit_group";
		syscall_names[273] = "set_robust_list";
		syscall_names[302] = "prlimit64";
		syscall_names[257] = "openat";
		syscall_names[293] = "pipe2";
		syscall_names[318] = "getrandom";
		syscall_names[334] = "rseq";
	}
	if (nr >= 0 && nr < 335)
		return syscall_names[nr];
	return NULL;
}

void print_buf(const char *buf, size_t n) {
	for (size_t i = 0; i < n; i++) {
		unsigned char c = (unsigned char)buf[i];
		if ((c >= 32 && c < 127) || c == '\n' || c == '\t' || c == '\r')
			putchar(c);
		else
			putchar('.');
	}
}

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
	long last_nr = -1;

	while (1) {
		if (pr_get_state(prs, pid) == PR_BLOCKED) {
			int st;
			usleep(2000);
			int r = waitpid(pid, &st, WNOHANG);
			if (r > 0) {
				if (WIFEXITED(st) || WIFSIGNALED(st)) {
					printf("[%d] proceso ha terminado con codigo %d\n", pid, WEXITSTATUS(st));
					pr_set_state(prs, pid, PR_TERMINATED);
					return;
				}
				pr_set_state(prs, pid, PR_STOPPED);
			} else {
				return;
			}
		}

		ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		waitpid(pid, &status, 0);

		if (WIFEXITED(status) || WIFSIGNALED(status)) {
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
				printf("[%d] %s() del fichero %s\n", pid, syscall_name(syscall_nr), fname);
				if (fname) free(fname);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_write: {
				size_t count = regs.rdx;
				char buf[count];
				peek_proc_mem(pid, (void*)regs.rsi, buf, count);
				printf("[%d] %s() con argumentos: descriptor de fichero %d, tamaño %ld, datos=\"",
					pid, syscall_name(syscall_nr), (int)regs.rdi, count);
				print_buf(buf, count);
				printf("\"\n");
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_read: {
				printf("[%d] %s() con argumentos: descriptor de fichero %d, tamaño %llu\n",
					pid, syscall_name(syscall_nr), (int)regs.rdi, regs.rdx);
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
				ptrace(PTRACE_GETREGS, pid, NULL, &regs);
				if ((long)regs.rax >= 0) {
					printf("[%d] exec() exitoso\n", pid);
					ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
					waitpid(pid, &status, 0);
				} else {
					printf("[%d] exec() falló: %s\n", pid, strerror(-(long)regs.rax));
				}
				pr_set_state(prs, pid, PR_STOPPED);
				return;
			}
			case SYS_dup:
			case SYS_dup2: {
				if (syscall_nr == SYS_dup2)
					printf("[%d] %s() con argumentos: oldfd=%d, newfd=%d\n",
						pid, syscall_name(syscall_nr), (int)regs.rdi, (int)regs.rsi);
				else
					printf("[%d] %s() con argumentos: oldfd=%d\n",
						pid, syscall_name(syscall_nr), (int)regs.rdi);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_pipe:
			case SYS_pipe2: {
				printf("[%d] %s() con argumentos: buffer con direccion %p\n",
					pid, syscall_name(syscall_nr), (void*)regs.rdi);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			case SYS_socket: {
				printf("[%d] %s() argumentos: domain=%d, type=%d, protocol=%d\n",
					pid, syscall_name(syscall_nr), (int)regs.rdi, (int)regs.rsi, (int)regs.rdx);
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
				pr_set_state(prs, pid, PR_STOPPED);
				return;
			}
			case SYS_wait4: {
				printf("[%d] %s()\n", pid, syscall_name(syscall_nr));
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
				return;
			}
			default: {
				last_nr = syscall_nr;
				char* name = syscall_name(syscall_nr);
				if (name)
					printf("[%d] syscall no de interes: %s()\n", pid, name);
				else
					printf("[%d] syscall no de interes: %ld\n", pid, syscall_nr);
				ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
				pr_set_state(prs, pid, PR_BLOCKED);
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
			printf("[%d] fin %s() retorna %lld\n", pid, syscall_name(syscall_nr), regs.rax);
			break;
		case SYS_write:
			printf("[%d] fin %s() retorna %lld\n", pid, syscall_name(syscall_nr), regs.rax);
			break;
		case SYS_read: {
			long count = (long)regs.rax;
			printf("[%d] fin %s() retorna %ld", pid, syscall_name(syscall_nr), count);
			if (count > 0) {
				char buf[count];
				peek_proc_mem(pid, (void*)regs.rsi, buf, count);
				printf(", datos=\"");
				print_buf(buf, count);
				printf("\"");
			}
			printf("\n");
			break;
		}
		case SYS_dup:
		case SYS_dup2:
			printf("[%d] fin %s() retorna %lld\n", pid, syscall_name(syscall_nr), regs.rax);
			break;
		case SYS_pipe:
		case SYS_pipe2: {
			long ret = (long)regs.rax;
			if (ret == 0) {
				int fd[2];
				peek_proc_mem(pid, (void*)regs.rdi, fd, sizeof(fd));
				printf("[%d] fin %s() descriptores creados: fd lectura %d, fd escritura %d\n",
					pid, syscall_name(syscall_nr), fd[0], fd[1]);
			}
			printf("[%d] fin %s() retorna %ld\n", pid, syscall_name(syscall_nr), ret);
			break;
		}
		case SYS_socket:
			printf("[%d] fin %s() retorna descriptor %lld\n", pid, syscall_name(syscall_nr), regs.rax);
			break;
		case SYS_wait4:
			if ((long)regs.rax == -512)
				printf("[%d] fin %s() interrumpido por señal, necesita reiniciarse\n", pid, syscall_name(syscall_nr));
			else
				printf("[%d] fin %s() retorna %lld\n", pid, syscall_name(syscall_nr), regs.rax);
			break;
		default:
			break;
	}

	pr_set_state(prs, pid, PR_STOPPED);
}
