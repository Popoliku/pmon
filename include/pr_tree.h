#ifndef PR_TREE_H
#define PR_TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct {
	pid_t pid;   
	pid_t ppid;  
} pr_node;

typedef struct {
	pr_node* nodes;    
	size_t size;    
	size_t capacity; 
} pr_array;

void add_pr(pr_array* prs, pid_t pid, pid_t ppid);

void print_tree(pr_array* prs, pid_t current_pid, int level);

#endif
