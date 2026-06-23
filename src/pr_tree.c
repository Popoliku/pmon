#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "fd_table.h"

typedef struct {
	pid_t pid;
	pid_t ppid;
} pr_node;

typedef struct {
	pr_node* nodes;
	size_t size;
	size_t capacity;
} pr_array;

void add_pr(pr_array* prs, pid_t pid, pid_t ppid) {
	if(prs->size == prs->capacity) {
		prs->capacity = (prs->capacity == 0) ? 10 : prs->capacity*2;
		prs->nodes = realloc(prs->nodes, prs->capacity*sizeof(pr_node));
	}
	prs->nodes[prs->size].pid = pid;
	prs->nodes[prs->size].ppid = ppid;
	prs->size++;
}

void print_tree(FILE* stream, pr_array* prs, pid_t current_pid, int level) {
	for(int i = 0; i<prs->size; i++) {
		if(prs->nodes[i].ppid == current_pid) {
			for(int j = 0; j < level; j++) {
				fprintf(stream, "      ");             
			}
			fprintf(stream, "└── %d\n", prs->nodes[i].pid);
			print_fds(stream, prs->nodes[i].pid, level+1);
			print_tree(stream, prs, prs->nodes[i].pid, level + 1);
		}
	}
}
