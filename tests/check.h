/**
 * @file check.h
 * @brief Minimal C test harness: CHECK records failures, main returns their count.
 */

#ifndef AUTHUM_TESTS_CHECK_H
#define AUTHUM_TESTS_CHECK_H

#include <stdio.h>

extern int authum_check_failures;
extern int authum_check_total;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        ++authum_check_total;                                                                      \
        if (!(cond)) {                                                                             \
            ++authum_check_failures;                                                               \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);               \
        }                                                                                          \
    } while (0)

#define CHECK_STR(a, b) CHECK(strcmp((a), (b)) == 0)

/** Print a summary; returns the process exit code. */
int
authum_check_report(const char *suite);

#endif /* AUTHUM_TESTS_CHECK_H */
