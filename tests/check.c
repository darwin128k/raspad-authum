#include <check.h>

int authum_check_failures;
int authum_check_total;

int
authum_check_report(const char *suite)
{
    printf("%s: %d/%d checks passed\n", suite, authum_check_total - authum_check_failures,
           authum_check_total);
    return authum_check_failures == 0 ? 0 : 1;
}
