#include <unistd.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Número de argumentos incorrecto\n");
		return 1;
	}
	pid_t pid = fork();
	if(pid == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);	
		raise(SIGSTOP);
		execvp(argv[1], &argv[1]);
		perror("Error en el exec");
		return 1;
	} 
	int status;
	//int syscall_nr;
	//struct user_regs_struct regs;
	waitpid(pid, &status, 0);
	ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACEFORK);
	while(1) {
		//ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	}
}
