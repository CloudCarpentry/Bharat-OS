#ifndef BHARAT_COMPILER_SAFETY_H
#define BHARAT_COMPILER_SAFETY_H

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if defined(__clang__)
  #define BHARAT_COMPILER_CLANG 1
#else
  #define BHARAT_COMPILER_CLANG 0
#endif

#if __has_attribute(noinline)
  #define BHARAT_NOINLINE __attribute__((noinline))
#else
  #define BHARAT_NOINLINE
#endif

#if __has_attribute(used)
  #define BHARAT_USED __attribute__((used))
#else
  #define BHARAT_USED
#endif

#if __has_builtin(__builtin_memcpy)
  #define BHARAT_HAS_BUILTIN_MEMCPY 1
#else
  #define BHARAT_HAS_BUILTIN_MEMCPY 0
#endif

#endif /* BHARAT_COMPILER_SAFETY_H */
