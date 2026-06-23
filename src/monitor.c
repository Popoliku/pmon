#include <unistd.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include "input.h"
#include "pr_tree.h"
#include "syscall_handler.h"

int quit_cmd(pr_array* prs) {
	for(int i = 0;  i<prs->size; i++) {
		if(prs->nodes[i].alive) {
			pid_t pid = prs->nodes[i].pid;
			kill(pid, SIGKILL);
			waitpid(pid, NULL, WNOHANG);
		}
	}
	if(prs->nodes != NULL) free(prs->nodes);
	return(0);
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
	pr_array prs = { NULL, 0, 0 };
	waitpid(child_pid, &status, 0);
	printf("El pid del proceso monitorizado es %d\n", child_pid);
	add_pr(&prs, child_pid, -1);
	ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACEFORK | PTRACE_O_TRACESYSGOOD);
	while(1) {
		printf("pmon> ");
		fflush(stdout);
		char* command = NULL;
		int pmon_argc = 0;
		char** pmon_argv = get_input(stdin, &command, &pmon_argc);
		if(pmon_argc == 0) continue;
		if(strcmp(pmon_argv[0], "cont") == 0) {
			if(pmon_argc != 2) {
				printf("El uso correcto del comando es: cont <pid>\n");
			} else {
				pid_t pid = atoi(pmon_argv[1]);
				int state = pr_state(&prs, pid);
				if(state == -1) {
					printf("No existe proceso con pid %d\n", pid);
					continue;
				} else if(!state) {
					printf("[%d] proceso ya terminado\n", pid);
					continue;
				}
				int syscall_interes = 0;
				struct user_regs_struct regs;
				while(!syscall_interes) {
					ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
					waitpid(pid, &status, 0);
					ptrace(PTRACE_GETREGS, pid, NULL, &regs);
					if(WIFEXITED(status)) {
						printf("[%d] proceso ha terminado con codigo %d\n", pid, WEXITSTATUS(status));
						mark_term(&prs, pid);
						ptrace(PTRACE_DETACH, pid, NULL, NULL);
						break;
					}
					if (WIFSTOPPED(status)) {
						int signal = WSTOPSIG(status);
						if (signal == SIGCHLD) {
							continue; 
						}
					}
					syscall_interes = handle_syscall(pid, &status, &regs, &prs);
				}
			}
			free(pmon_argv);
			free(command);
		} else if(strcmp(pmon_argv[0], "ps") == 0) {
			print_tree(stdout, &prs, -1, 0);
		} else if(strcmp(pmon_argv[0], "quit") == 0) {
			return quit_cmd(&prs);	
		} else {
			printf("Comando inexistente\n");
		}
	}
}
