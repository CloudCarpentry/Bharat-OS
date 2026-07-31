#include <standard/assert.h>
#include <standard/stddef.h>
#include <standard/string.h>
#include <bharat/bsys/backend.h>

#ifdef BHARATLIBC_HOST_MODE
#include <stdlib.h>
#include <stdio.h>
#endif

static bh_assert_hook_t g_assert_hook = NULL;

void bh_assert_set_hook(bh_assert_hook_t hook) {
    g_assert_hook = hook;
}

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function) {
    (void)function;

    if (g_assert_hook) {
        g_assert_hook(assertion, file, (int)line);
    }

#ifdef BHARATLIBC_HOST_MODE
    fprintf(stderr, "Assertion failed: %s, file %s, line %d\n",
            assertion ? assertion : "unknown",
            file ? file : "unknown",
            (int)line);
    exit(134);
#else
    const bh_bsys_backend_ops_t *ops = bh_bsys_get_backend();
    if (ops && ops->write) {
        uint32_t written = 0;
        ops->write(2, "Assertion failed: ", 18, &written);
        if (assertion) {
            ops->write(2, assertion, (uint32_t)strlen(assertion), &written);
        }
        ops->write(2, " at ", 4, &written);
        if (file) {
            ops->write(2, file, (uint32_t)strlen(file), &written);
        }
        ops->write(2, "\n", 1, &written);
    }

    if (ops && ops->process_exit) {
        ops->process_exit(134); /* SIGABRT-like exit status */
    }

    /* Fallback infinite loop if exit is unsupported or does not return */
    while (1) {
        __asm__ __volatile__("");
    }
#endif
}
