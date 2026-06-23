#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

void print_fds(pid_t pid, int level) {
	char path[256];
	snprintf(path, sizeof(path), "/proc/%d/fd", pid);
	DIR *dir = opendir(path);
	if (!dir) return;
	struct dirent *entry;
	int fds[512]; 
	int n = 0;
	while ((entry = readdir(dir)) != NULL && n<512) {
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
		ssize_t len = readlink(link, target, sizeof(target) - 1);
		if (len == -1) continue;
		target[len] = '\0';
		for(int j = 0; j<level; j++) {
			printf("      ");
		}
		if (strncmp(target, "pipe:", 5) == 0) {
			printf("│ FD %d --> %s\n", fd, target);
		} else {
			printf("│ FD %d --> %s\n", fd, target);
		}
	}
}
