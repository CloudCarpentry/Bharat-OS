#ifndef BHARATLIBC_STDDEF_H
#define BHARATLIBC_STDDEF_H

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WCHAR_TYPE__ wchar_t;

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif /* BHARATLIBC_STDDEF_H */
