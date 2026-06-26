#include <unistd.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <pthread.h>
#include "input.h"
#include "pr_tree.h"
#include "syscall_handler.h"

#define CMD_CONT 1
#define CMD_PS   2
#define CMD_QUIT 3
#define CMD_SEND 4

typedef struct {
	int tipo;
	pid_t pid;
	char data[256];
} pmon_cmd;

int cmd_pipe[2];
int ready_pipe[2];
pr_array prs;

void quit_cmd() {
	for (int i = 0; i < prs.size; i++) {
		if (prs.nodes[i].state != PR_TERMINATED) {
			kill(prs.nodes[i].pid, SIGKILL);
			waitpid(prs.nodes[i].pid, NULL, 0);
		}
	}
	if (prs.nodes != NULL) free(prs.nodes);
}

void* ui_thread() {
	while (1) {
		char r;
		read(ready_pipe[0], &r, 1);
		printf("pmon> ");
		fflush(stdout);
		char* command = NULL;
		int pmon_argc = 0;
		char** pmon_argv = get_input(stdin, &command, &pmon_argc);
		if (pmon_argc == 0) {
			free(pmon_argv);
			free(command);
			continue;
		}

		pmon_cmd cmd = {0, 0, {0}};

		if (strcmp(pmon_argv[0], "cont") == 0) {
			if (pmon_argc != 2) {
				printf("El uso correcto del comando es: cont <pid>\n");
			} else {
				cmd.tipo = CMD_CONT;
				cmd.pid = atoi(pmon_argv[1]);
				write(cmd_pipe[1], &cmd, sizeof(cmd));
			}
		} else if (strcmp(pmon_argv[0], "ps") == 0) {
			cmd.tipo = CMD_PS;
			write(cmd_pipe[1], &cmd, sizeof(cmd));
		} else if (strcmp(pmon_argv[0], "send") == 0) {
			if (pmon_argc < 3) {
				printf("El uso correcto del comando es: send <pid> <datos>\n");
			} else {
				cmd.tipo = CMD_SEND;
				cmd.pid = atoi(pmon_argv[1]);
				for (int i = 2; i < pmon_argc; i++) {
					strncat(cmd.data, pmon_argv[i], sizeof(cmd.data) - strlen(cmd.data) - 1);
					if (i < pmon_argc - 1)
						strncat(cmd.data, " ", sizeof(cmd.data) - strlen(cmd.data) - 1);
				}
				strncat(cmd.data, "\n", sizeof(cmd.data) - strlen(cmd.data) - 1);
				write(cmd_pipe[1], &cmd, sizeof(cmd));
			}
		} else if (strcmp(pmon_argv[0], "quit") == 0) {
			cmd.tipo = CMD_QUIT;
			write(cmd_pipe[1], &cmd, sizeof(cmd));
			free(pmon_argv);
			free(command);
			return NULL;
		} else {
			printf("Comando inexistente\n");
			write(ready_pipe[1], "R", 1);
		}

		free(pmon_argv);
		free(command);
	}
	return NULL;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Número de argumentos incorrecto\n");
		return 1;
	}

	int child_stdin[2];
	pipe(child_stdin);

	pid_t child_pid = fork();
	if (child_pid == 0) {
		dup2(child_stdin[0], STDIN_FILENO);
		close(child_stdin[0]);
		close(child_stdin[1]);
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		raise(SIGSTOP);
		execvp(argv[1], &argv[1]);
		perror("Error en el exec");
		return 1;
	}
	close(child_stdin[0]);
	int status;
	prs = (pr_array){ NULL, 0, 0 };
	waitpid(child_pid, &status, 0);
	printf("El pid del proceso monitorizado es %d\n", child_pid);
	add_pr(&prs, child_pid, -1);
	ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACEFORK | PTRACE_O_TRACESYSGOOD);

	pipe(cmd_pipe);
	fcntl(cmd_pipe[0], F_SETFL, O_NONBLOCK);
	pipe(ready_pipe);
	write(ready_pipe[1], "R", 1);

	pthread_t ui;
	pthread_create(&ui, NULL, ui_thread, NULL);

	while (1) {
		int listo = 0;
		pmon_cmd cmd;
		if (read(cmd_pipe[0], &cmd, sizeof(cmd)) > 0) {
			if (cmd.tipo == CMD_QUIT) {
				quit_cmd();
				exit(0);
			} else if (cmd.tipo == CMD_PS) {
				print_tree(stdout, &prs, -1, 0);
			} else if (cmd.tipo == CMD_CONT) {
				int state = pr_get_state(&prs, cmd.pid);
				if (state == -1) {
					printf("No existe proceso con pid %d\n", cmd.pid);
				} else if (state == PR_TERMINATED) {
					printf("[%d] proceso ya terminado\n", cmd.pid);
				} else if (state == PR_BLOCKED) {
					printf("[%d] proceso en syscall bloqueante, espere\n", cmd.pid);
				} else {
					handle_syscall_entrada(cmd.pid, &prs);
				}
			} else if (cmd.tipo == CMD_SEND) {
				int state = pr_get_state(&prs, cmd.pid);
				if (state == -1) {
					printf("No existe proceso con pid %d\n", cmd.pid);
				} else if (state == PR_TERMINATED) {
					printf("[%d] proceso ya terminado\n", cmd.pid);
				} else {
					write(child_stdin[1], cmd.data, strlen(cmd.data));
				}
			}
			listo = 1;
		}
		usleep(1000);
		for (int i = 0; i < prs.size; i++) {
			if (prs.nodes[i].state != PR_BLOCKED) continue;
			pid_t pid = prs.nodes[i].pid;
			int st;
			if (waitpid(pid, &st, WNOHANG) > 0) {
				handle_syscall_salida(pid, &st, &prs);
				listo = 1;
			}
		}
		if (listo) write(ready_pipe[1], "R", 1);
	}
}
