#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

int fd_mode(pid_t pid, int fd) {
	char path[256];
	snprintf(path, sizeof(path), "/proc/%d/fdinfo/%d", pid, fd);
	FILE* f = fopen(path, "r");
	char line[256];
	int flags = -1;
	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "flags:", 6) == 0) {
			flags = (int)strtol(line + 6, NULL, 8);
			break;
		}
	}
	fclose(f);
	return flags & 1;
}

void print_fds(FILE* stream, pid_t pid, int level) {
	char path[256];
	snprintf(path, sizeof(path), "/proc/%d/fd", pid);
	DIR* dir = opendir(path);
	if (!dir) return;
	struct dirent* entry;
	int fds[256]; 
	int n = 0;
	while ((entry = readdir(dir)) != NULL && n<256) {
		if (entry->d_name[0] == '.') continue;
		fds[n++] = atoi(entry->d_name);
	}
	closedir(dir);
	for (int i = 0; i<n-1; i++) {
		for (int j = i+1; j<n; j++) {
			if (fds[i]>fds[j]) { 
				int temp = fds[i]; 
				fds[i] = fds[j]; 
				fds[j] = temp; 
			}
		}
	}
	for (int i = 0; i<n; i++) {
		int fd = fds[i];
		char link[256], target[256];
		snprintf(link, sizeof(link), "/proc/%d/fd/%d", pid, fd);
		int len = readlink(link, target, sizeof(target) - 1);
		if (len == -1) continue;
		target[len] = '\0';
		for(int j = 0; j<level; j++) {
			fprintf(stream, "      ");
		}
		if (strncmp(target, "pipe:", 5) == 0) {
			int mode = fd_mode(pid, fd);
			if(mode) fprintf(stream, "│ FD %d --> extremo de escritura %s\n", fd, target);
			else fprintf(stream, "│ FD %d --> extremo de lecetura %s\n", fd, target);
		} else {
			fprintf(stream, "│ FD %d --> %s\n", fd, target);
		}
	}
}
