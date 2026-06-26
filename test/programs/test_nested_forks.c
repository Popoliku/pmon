/*
 * test_nested_forks.c
 *
 * Programa de prueba para el monitor pmon.
 *
 * Jerarquia de procesos y syscalls de interes:
 *
 *   P0 (padre)     -> open() + write() + fork(P1) + wait(P1)
 *   P1 (hijo)      -> socket() + read(stdin) [BLOQUEANTE] + fork(P2) + wait(P2)
 *   P2 (nieto)     -> pipe() + dup2() + fork(P3) + wait(P3)
 *   P3 (bisnieto)  -> execve()
 *
 * P1 se bloquea en read(0,...) esperando datos por stdin.
 * Usar el comando: send <pid_P1> <datos>
 *
 * Uso:
 *   ./pmon ./test_nested_forks
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
    const char *msg = "nieto write\n";
    write(1, msg, strlen(msg));
    waitpid(pid, NULL, 0);
    close(pipefd[0]);
    close(pipefd[1]);
    close(10);
    return 0;
}

int hijo() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) close(sock);

    char buf[64];
    ssize_t n = read(0, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        write(1, buf, n);
    }

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

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork padre");
        return 1;
    }
    if (pid1 == 0) {
        return hijo();
    }
    waitpid(pid1, NULL, 0);
    unlink(TMP_FILE);
    return 0;
}
