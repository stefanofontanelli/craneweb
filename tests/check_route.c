/**************************************************************************
 * check_route: craneweb routes test suite.                               *
 **************************************************************************/
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <check.h>

#include "config.h"

#include "craneweb.h" 
#include "craneweb_private.h" 


/*************************************************************************/

START_TEST(test_none)
{
    CRW_Route *R = CRW_route_new();
    fail_if(R == NULL, "failed to allocate a route object");
    CRW_route_del(R);
}
END_TEST


/*************************************************************************/

TCase *craneweb_testCaseRoute(void)
{
    TCase *tcRoute = tcase_create("craneweb.core.route");
    tcase_add_test(tcRoute, test_none);

    return tcRoute;
}

static Suite *craneweb_suiteRoute(void)
{
    TCase *tc = craneweb_testCaseRoute();
    Suite *s = suite_create("craneweb.core.route");
    suite_add_tcase(s, tc);
    return s;
}

/*************************************************************************/

int main(int argc, char *argv[])
{
    int number_failed = 0;

    Suite *s = craneweb_suiteRoute();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*************************************************************************/

/* vim: set ts=4 sw=4 et */
/* EOF */

