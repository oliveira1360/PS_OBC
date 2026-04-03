#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>

#define ASSERT(cond, msg) \
    if (cond) { printf("  PASS: %s\n", msg); } \
    else       { printf("  FAIL: %s\n", msg); }

#define TEST(name) printf("\n[TEST] %s\n", name)

#endif