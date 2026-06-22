#include <criterion/criterion.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "syscall_handler.h"

Test(peek_proc_string, test_with_open) {
	pid_t pid = fork();
	if (pid == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		raise(SIGSTOP);
		open("archivo_inexistente.txt", O_RDONLY);
		exit(0);
	} else {
		printf("4\n");
		int status;
		struct user_regs_struct regs;

		waitpid(pid, &status, 0); 
		ptrace(PTRACE_SYSCALL, pid, NULL, NULL); 
		waitpid(pid, &status, 0);
		printf("5\n");

		ptrace(PTRACE_GETREGS, pid, NULL, &regs);
		void *pr_addr = (void *)regs.rsi;

		char *fname = peek_proc_string(pid, pr_addr);

		cr_assert_not_null(fname);
		cr_assert_str_eq(fname, "archivo_inexistente.txt");

		printf("6\n");
		free(fname);
		ptrace(PTRACE_CONT, pid, NULL, NULL);
		waitpid(pid, &status, 0);
	}
}

Test(peek_proc_mem, test_with_open) {
	pid_t pid = fork();

	if (pid == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		raise(SIGSTOP);
		open("archivo_inexistente.txt", O_RDONLY);
		exit(0);
	} else {
		int status;
		struct user_regs_struct regs;

		waitpid(pid, &status, 0); 
		ptrace(PTRACE_SYSCALL, pid, NULL, NULL); 
		waitpid(pid, &status, 0);

		ptrace(PTRACE_GETREGS, pid, NULL, &regs);
		void *pr_addr = (void *)regs.rsi;

		char buf[8];
		buf[7] = '\0';

		peek_proc_mem(pid, pr_addr, buf, 7);

		cr_assert_str_eq(buf, "archivo");

		ptrace(PTRACE_CONT, pid, NULL, NULL);
		waitpid(pid, &status, 0);
	}
}
