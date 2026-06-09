#include <unistd.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

char** get_input(FILE* stream, char** comando, int* pmon_argc) {
	size_t capacidad = 0;
	if(getline(comando, &capacidad, stream) == -1 && !feof(stream)) {
		perror("Error leyendo entrada");
	}
	char* ptr = strchr(*comando, '\n');
	*ptr = 0;
	capacidad = 10;
	int argc = 0;
	char** argv = malloc(capacidad*sizeof(char*)); 
	char* token = strtok(*comando, " ");
	while(token != NULL) {
		if(argc == (int)capacidad) {
			capacidad *= 2;
			argv = realloc(argv, capacidad*sizeof(char*));
		}
		argv[argc++] = token;
		token = strtok(NULL, " ");
	}
	argv[argc] = NULL;
	*pmon_argc = argc;
	return argv;
}

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
		printf("pmon> ");
		fflush(stdout);
		char* comando = NULL;
		int pmon_argc = 0;
		char** pmon_argv = get_input(stdin, &comando, &pmon_argc);
		for(int i = 0; i<pmon_argc; i++) {
			printf("%s ", pmon_argv[i]); 
		}
		printf("\n");
		if(pmon_argc > 2) printf("Invalid number of arguments\n");
		free(pmon_argv);
		free(comando);
		//ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	}
}
