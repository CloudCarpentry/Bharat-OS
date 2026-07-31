#ifndef BHARATLIBC_ASSERT_H
#define BHARATLIBC_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bh_assert_hook_t)(const char *expression, const char *file, int line);

void bh_assert_set_hook(bh_assert_hook_t hook);
void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) \
    ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_ASSERT_H */
