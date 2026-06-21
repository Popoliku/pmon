/*
 * test_nested_forks.c
 *
 * Programa de prueba para el monitor pmon.
 *
 * Cuatro procesos en los que se intenta abarcar todos los syscall de interes, 
 * en este orden:
 *
 *   P0 (padre)     -> open() + write()  
 *   P1 (hijo)      -> open() + read()   
 *   P2 (nieto)     -> pipe() + dup2()   
 *   P3 (bisnieto)  -> execve()         
 *
 * Cada proceso espera a que su hijo termine antes de terminar el mismo,
 * por lo que no se recomienda hacer cont de un proceso si la siguiente syscall
 * es un wait().
 *
 * Uso:
 *   ./pmon ./test_nested_forks
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define TMP_FILE "/tmp/pmon_test_nested.txt"

int bisnieto() {
    char *args[] = {"/bin/echo", "bisnieto execve", NULL};
    execve("/bin/echo", args, NULL);
    perror("execve");
    return 1;
}

int nieto() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    } 
    if (dup2(pipefd[1], 10) == -1) {
        perror("dup2");
        return 1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork nieto");
        return 1;
    }
    if (pid == 0) {
        return bisnieto();
    }
    waitpid(pid, NULL, 0);
    close(pipefd[0]);
    close(pipefd[1]);
    close(10);
    return 0;
}

int hijo() {
    int fd = open(TMP_FILE, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    char buf[64];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
    }
    close(fd);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork hijo");
        return 1;
    }
    if (pid == 0) {
        return nieto();
    }
    waitpid(pid, NULL, 0);
    return 0;
}

int main(void) {
    int fd = open(TMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    const char *contenido = "padre write\n";
    write(fd, contenido, strlen(contenido));
    close(fd);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork padre");
        return 1;
    }
    if (pid == 0) {
        return hijo();
    }    
    const char *msg = "padre write 2\n";
    write(1, msg, strlen(msg));
    waitpid(pid, NULL, 0);
    unlink(TMP_FILE);
    return 0;
}
