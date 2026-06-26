#ifndef PR_TREE_H
#define PR_TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define PR_STOPPED    0
#define PR_BLOCKED    1
#define PR_TERMINATED 2

typedef struct {
	pid_t pid;
	pid_t ppid;
	int state;
} pr_node;

typedef struct {
	pr_node* nodes;
	size_t size;
	size_t capacity;
} pr_array;

void add_pr(pr_array* prs, pid_t pid, pid_t ppid);
void pr_set_state(pr_array* prs, pid_t pid, int state);
int pr_get_state(pr_array* prs, pid_t pid);
void print_tree(FILE* stream, pr_array* prs, pid_t current_pid, int level);

#endif
