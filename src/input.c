#include <stdio.h>
#include <stdlib.h>
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
