#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_registry.h"

int main(void) {
    size_t count = 0;
    size_t i;
    int found_help = 0;
    int found_sys_info = 0;
    const shell_command_entry_t* entries = shell_registry_get(&count);

    assert(entries != NULL);
    assert(count >= 10);

    for (i = 0; i < count; ++i) {
        if (strcmp(entries[i].command, "help") == 0) {
            found_help = 1;
        }
        if (strcmp(entries[i].command, "sys info") == 0) {
            found_sys_info = 1;
        }
    }

    assert(found_help == 1);
    assert(found_sys_info == 1);
    printf("test_shell_registry passed\n");
    return 0;
}
