#include <criterion/criterion.h>
#include <stdio.h>
#include "pr_tree.h"

Test(add_pr, array_not_full) {
	pr_array prs = { NULL, 0, 0 };

	add_pr(&prs, 0, -1);

	cr_assert_eq(prs.size, 1);
	cr_assert_geq(prs.capacity, 10);
	cr_assert_eq(prs.nodes[0].pid, 0);
	cr_assert_eq(prs.nodes[0].ppid, -1);

	free(prs.nodes);
}

Test(add_pr, array_full) {
	pr_array prs = { NULL, 0, 0 };

	for(int i = 0; i<11; i++) {
		add_pr(&prs, i, i-1);
	}

	cr_assert_eq(prs.size, 11);
	cr_assert_geq(prs.capacity, 20);
	for(int i = 0; i<11; i++) cr_assert_eq(prs.nodes[i].pid, i);
	for(int i = 0; i<11; i++) cr_assert_eq(prs.nodes[i].ppid, i-1);

	free(prs.nodes);
}

Test(print_tree, empty_tree) {
	pr_array prs = { NULL, 0, 0 };

	char* buffer = NULL;
	size_t size = 0;
	FILE* mem_stream = open_memstream(&buffer, &size);

	print_tree(mem_stream, &prs, -1, 0);
	fflush(mem_stream);
	fclose(mem_stream);

	cr_assert_str_eq(buffer, "");

	free(buffer);
}

Test(print_tree, not_empty_tree) {
    pr_array prs = { NULL, 0, 0 };

    add_pr(&prs, 1000, -1);
    add_pr(&prs, 2000, 1000);

    char* buffer = NULL;
    size_t size = 0;
    FILE *mem_stream = open_memstream(&buffer, &size);

    print_tree(mem_stream, &prs, -1, 0);
    fflush(mem_stream);
    fclose(mem_stream);

    cr_assert_str_eq(buffer, "└── 1000\n      └── 2000\n");

    free(buffer);
    free(prs.nodes);
}
