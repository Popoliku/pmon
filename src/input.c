#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char** get_input(FILE* stream, char** command, int* pmon_argc) {
	size_t capacity = 0;
	if(getline(command, &capacity, stream) == -1 && !feof(stream)) {
		perror("Error leyendo entrada");
	}
	char* ptr = strchr(*command, '\n');
	*ptr = 0;
	capacity = 10;
	int argc = 0;
	char** argv = malloc(capacity*sizeof(char*)); 
	char* token = strtok(*command, " ");
	while(token != NULL) {
		if(argc == (int)capacity) {
			capacity *= 2;
			argv = realloc(argv, capacity*sizeof(char*));
		}
		argv[argc++] = token;
		token = strtok(NULL, " ");
	}
	argv[argc] = NULL;
	*pmon_argc = argc;
	return argv;
}
