#include <criterion/criterion.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include "fd_table.h"

Test(print_fds, without_pipes) {
    pid_t pid = getpid();

    char* buffer = NULL;
    size_t size = 0;
    FILE* mem_stream = open_memstream(&buffer, &size);

    print_fds(mem_stream, pid, 0);
    fflush(mem_stream);
    fclose(mem_stream);

    cr_assert_not_null(buffer);
    cr_assert_not_null(strstr(buffer, "│ FD 0 -->"));
    cr_assert_not_null(strstr(buffer, "│ FD 1 -->"));

    free(buffer);
}
