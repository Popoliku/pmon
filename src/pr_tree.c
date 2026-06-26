#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "pr_tree.h"
#include "fd_table.h"

void add_pr(pr_array* prs, pid_t pid, pid_t ppid) {
	if(prs->size == prs->capacity) {
		prs->capacity = (prs->capacity == 0) ? 10 : prs->capacity*2;
		prs->nodes = realloc(prs->nodes, prs->capacity*sizeof(pr_node));
	}
	prs->nodes[prs->size].pid = pid;
	prs->nodes[prs->size].ppid = ppid;
	prs->nodes[prs->size].state = PR_STOPPED;
	prs->size++;
}

void pr_set_state(pr_array* prs, pid_t pid, int state) {
	for(int i = 0; i<prs->size; i++) {
		if(prs->nodes[i].pid == pid) {
			prs->nodes[i].state = state;
			return;
		}
	}
}

int pr_get_state(pr_array* prs, pid_t pid) {
	for(int i = 0; i<prs->size; i++) {
		if(prs->nodes[i].pid == pid) return prs->nodes[i].state;
	}
	return -1;
}


void print_tree(FILE* stream, pr_array* prs, pid_t current_pid, int level) {
	for(int i = 0; i<prs->size; i++) {
		if(prs->nodes[i].ppid == current_pid) {
			for(int j = 0; j < level; j++) {
				fprintf(stream, "      ");
			}
			fprintf(stream, "└── %d", prs->nodes[i].pid);
			if(prs->nodes[i].state == PR_TERMINATED) fprintf(stream, " (terminado)");
			else if(prs->nodes[i].state == PR_BLOCKED) fprintf(stream, " (bloqueado)");
			fprintf(stream, "\n");
			print_fds(stream, prs->nodes[i].pid, level+1);
			print_tree(stream, prs, prs->nodes[i].pid, level + 1);
		}
	}
}
