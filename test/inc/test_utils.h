/**
 * @file test_utils.h
 * @brief Framework de testes unitarios para o OBC CubeSat.
 *
 * Conformidade ECSS-E-ST-40C Rev.1 S5.5.3.2 - Software Unit Testing:
 *   - Boundary testing (n-1, n, n+1)
 *   - Error cases and messages
 *   - Global variable access
 *   - Out of range / stress testing
 *
 * Conformidade ECSS-E-ST-40C Rev.1 S5.8.3.5 - Verification of Code:
 *   - Consistent interfaces
 *   - Proper event sequences
 *   - Error handling verification
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <math.h>

/* --- Contadores globais de testes --- */
static int _tests_total  = 0;
static int _tests_passed = 0;
static int _tests_failed = 0;

/* --- Macros de teste --- */

#define TEST_SUITE(name) \
    printf("\n========================================\n"); \
    printf("  SUITE: %s\n", name); \
    printf("========================================\n")

#define TEST(name) \
    printf("\n[TEST] %s\n", name)

#define ASSERT(cond, msg) do { \
    _tests_total++; \
    if (cond) { \
        _tests_passed++; \
        printf("  PASS: %s\n", msg); \
    } else { \
        _tests_failed++; \
        printf("  FAIL: %s [%s:%d]\n", msg, __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_EQ(a, b, msg) do { \
    _tests_total++; \
    if ((a) == (b)) { \
        _tests_passed++; \
        printf("  PASS: %s\n", msg); \
    } else { \
        _tests_failed++; \
        printf("  FAIL: %s (got %d, expected %d) [%s:%d]\n", \
               msg, (int)(a), (int)(b), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_NEQ(a, b, msg) do { \
    _tests_total++; \
    if ((a) != (b)) { \
        _tests_passed++; \
        printf("  PASS: %s\n", msg); \
    } else { \
        _tests_failed++; \
        printf("  FAIL: %s (values should differ, both = %d) [%s:%d]\n", \
               msg, (int)(a), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_FLOAT_RANGE(val, lo, hi, msg) do { \
    _tests_total++; \
    float _v = (float)(val); \
    if (_v >= (float)(lo) && _v <= (float)(hi)) { \
        _tests_passed++; \
        printf("  PASS: %s (%.2f in [%.2f, %.2f])\n", msg, _v, (float)(lo), (float)(hi)); \
    } else { \
        _tests_failed++; \
        printf("  FAIL: %s (%.2f NOT in [%.2f, %.2f]) [%s:%d]\n", \
               msg, _v, (float)(lo), (float)(hi), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, eps, msg) do { \
    _tests_total++; \
    float _diff = fabsf((float)(a) - (float)(b)); \
    if (_diff <= (float)(eps)) { \
        _tests_passed++; \
        printf("  PASS: %s\n", msg); \
    } else { \
        _tests_failed++; \
        printf("  FAIL: %s (got %.4f, expected %.4f, diff=%.4f) [%s:%d]\n", \
               msg, (float)(a), (float)(b), _diff, __FILE__, __LINE__); \
    } \
} while(0)

#define TEST_SUMMARY() do { \
    printf("\n========================================\n"); \
    printf("  RESULTADOS: %d total, %d passed, %d failed\n", \
           _tests_total, _tests_passed, _tests_failed); \
    printf("========================================\n"); \
} while(0)

#define TEST_RETURN() (_tests_failed > 0 ? 1 : 0)

#endif /* TEST_UTILS_H */
