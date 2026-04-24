#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_parser.h"

int main(void) {
    shell_argv_t argv;
    char line1[] = "svc status console";
    assert(shell_parse_line(line1, &argv) == SHELL_RC_OK);
    assert(argv.count == 3);
    assert(strcmp(argv.tokens[0], "svc") == 0);
    assert(strcmp(argv.tokens[2], "console") == 0);

    char line2[] = {0x01,'b','a','d','\0'};
    assert(shell_parse_line(line2, &argv) == SHELL_RC_PARSE_ERROR);

    char long_line[300];
    memset(long_line, 'a', sizeof(long_line));
    long_line[sizeof(long_line) - 1] = '\0';
    assert(shell_parse_line(long_line, &argv) == SHELL_RC_PARSE_ERROR);

    printf("test_shell_parser passed\n");
    return 0;
}
