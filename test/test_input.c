#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <input.h>

Test(get_input, basic) {
    FILE* stream = fmemopen("primero segundo tercero\n", 24, "r");
    char* comando = NULL;
    int argc = 0;

    char** argv = get_input(stream, &comando, &argc);

    cr_assert_eq(argc, 3);
    cr_assert_str_eq(argv[0], "primero");
    cr_assert_str_eq(argv[1], "segundo");
    cr_assert_str_eq(argv[2], "tercero");
    cr_assert_null(argv[argc]);

    free(argv);
    free(comando);
    fclose(stream);
}

