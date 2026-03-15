#ifndef BHARAT_TEST_FRAMEWORK_H
#define BHARAT_TEST_FRAMEWORK_H

#include <stdio.h>

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        printf("Test failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
    } \
} while(0)

#endif
