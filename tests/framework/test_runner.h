/**
 * @file test_runner.h
 * @brief Minimal unit-test framework for embedded C (host-compiled).
 *
 * Usage:
 *   1. Call TEST_BEGIN("Suite Name") at the start of main().
 *   2. Register tests with RUN_TEST(function_name).
 *   3. Call TEST_END() at the end of main() — returns 0 on all pass, 1 otherwise.
 *
 * Each test is isolated via setjmp/longjmp: a failing assertion aborts only
 * the current test and the runner continues with the next one.
 *
 * Design goals:
 *   - Zero external dependencies (C99 + POSIX headers only)
 *   - Detailed failure messages: expected vs actual + file + line
 *   - Safe for space-system test suites (no dynamic allocation)
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

/* =========================================================================
 * Internal state — do not access directly
 * ========================================================================= */
static int        _pass_count   = 0;
static int        _fail_count   = 0;
static jmp_buf    _test_jmp;
static const char *_current_test = "(none)";

/* =========================================================================
 * Suite control
 * ========================================================================= */

/**
 * @brief Print suite header and reset counters.
 *        Call once at the start of main().
 */
#define TEST_BEGIN(suite_name)                                          \
    do {                                                                \
        _pass_count = 0;                                               \
        _fail_count = 0;                                               \
        printf("\n╔══════════════════════════════════════════════╗\n"); \
        printf("║  TEST SUITE: %-31s║\n", (suite_name));              \
        printf("╚══════════════════════════════════════════════╝\n");  \
    } while (0)

/**
 * @brief Print summary and return exit code (0=all pass, 1=any fail).
 *        Use as: return TEST_END();
 */
#define TEST_END()                                                          \
    do {                                                                    \
        printf("\n──────────────────────────────────────────────\n");       \
        printf("  Results: %d passed", _pass_count);                       \
        if (_fail_count > 0)                                               \
            printf(", %d FAILED", _fail_count);                            \
        else                                                               \
            printf(", 0 failed");                                          \
        printf("\n──────────────────────────────────────────────\n\n");    \
        return (_fail_count == 0) ? 0 : 1;                                 \
    } while (0)

/* =========================================================================
 * Test runner
 * ========================================================================= */

/**
 * @brief Run a single test function with isolation.
 *        A failing assertion inside fn() does NOT crash the suite.
 */
#define RUN_TEST(fn)                                            \
    do {                                                        \
        _current_test = #fn;                                   \
        if (setjmp(_test_jmp) == 0) {                          \
            fn();                                               \
            printf("  [ PASS ]  %s\n", #fn);                   \
            _pass_count++;                                      \
        } else {                                               \
            _fail_count++;                                      \
        }                                                       \
    } while (0)

/* =========================================================================
 * Assertion macros
 * ========================================================================= */

/**
 * @brief Fail unconditionally with a message.
 */
#define TEST_FAIL(msg)                                                      \
    do {                                                                    \
        printf("  [ FAIL ]  %s\n    ↳ %s  (line %d)\n",                   \
               _current_test, (msg), __LINE__);                             \
        longjmp(_test_jmp, 1);                                              \
    } while (0)

/**
 * @brief Assert a boolean condition is true.
 */
#define TEST_ASSERT(cond, msg)                          \
    do { if (!(cond)) TEST_FAIL(msg); } while (0)

/**
 * @brief Assert two ints are equal.
 */
#define TEST_ASSERT_EQUAL_INT(expected, actual, msg)                        \
    do {                                                                    \
        int _e = (int)(expected);                                           \
        int _a = (int)(actual);                                             \
        if (_e != _a) {                                                     \
            printf("  [ FAIL ]  %s\n    ↳ %s  (line %d)\n"                \
                   "       expected: %d\n"                                  \
                   "       actual:   %d\n",                                 \
                   _current_test, (msg), __LINE__, _e, _a);                \
            longjmp(_test_jmp, 1);                                          \
        }                                                                   \
    } while (0)

/**
 * @brief Assert two uint8_t values are equal (printed as hex).
 */
#define TEST_ASSERT_EQUAL_UINT8(expected, actual, msg)                      \
    do {                                                                    \
        uint8_t _e = (uint8_t)(expected);                                   \
        uint8_t _a = (uint8_t)(actual);                                     \
        if (_e != _a) {                                                     \
            printf("  [ FAIL ]  %s\n    ↳ %s  (line %d)\n"                \
                   "       expected: 0x%02X\n"                              \
                   "       actual:   0x%02X\n",                             \
                   _current_test, (msg), __LINE__, _e, _a);                \
            longjmp(_test_jmp, 1);                                          \
        }                                                                   \
    } while (0)

/**
 * @brief Assert two uint32_t values are equal (printed as hex).
 */
#define TEST_ASSERT_EQUAL_UINT32(expected, actual, msg)                     \
    do {                                                                    \
        uint32_t _e = (uint32_t)(expected);                                 \
        uint32_t _a = (uint32_t)(actual);                                   \
        if (_e != _a) {                                                     \
            printf("  [ FAIL ]  %s\n    ↳ %s  (line %d)\n"                \
                   "       expected: 0x%08X\n"                              \
                   "       actual:   0x%08X\n",                             \
                   _current_test, (msg), __LINE__,                          \
                   (unsigned)_e, (unsigned)_a);                             \
            longjmp(_test_jmp, 1);                                          \
        }                                                                   \
    } while (0)

/**
 * @brief Assert two byte arrays are equal.
 */
#define TEST_ASSERT_EQUAL_MEM(expected, actual, len, msg)                   \
    do {                                                                    \
        if (memcmp((expected), (actual), (len)) != 0) {                     \
            printf("  [ FAIL ]  %s\n    ↳ %s  (line %d) — buffers differ\n",\
                   _current_test, (msg), __LINE__);                         \
            longjmp(_test_jmp, 1);                                          \
        }                                                                   \
    } while (0)

/** @brief Assert condition is true. */
#define TEST_ASSERT_TRUE(cond, msg)  TEST_ASSERT((cond),  (msg))

/** @brief Assert condition is false. */
#define TEST_ASSERT_FALSE(cond, msg) TEST_ASSERT(!(cond), (msg))

/** @brief Assert pointer is not NULL. */
#define TEST_ASSERT_NOT_NULL(ptr, msg) TEST_ASSERT((ptr) != NULL, (msg))

/** @brief Assert pointer is NULL. */
#define TEST_ASSERT_NULL(ptr, msg) TEST_ASSERT((ptr) == NULL, (msg))

#endif /* TEST_RUNNER_H */
