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

START_TEST(test_regex_empty)
{
    CRW_Route *R = CRW_route_new();
    int err = CRW_route_init(R, "");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_clean1)
{
    CRW_Route *R = CRW_route_new();
    int err = CRW_route_init(R, "/");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected");
    CRW_route_tag_dump(R, "regex_clean1");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_clean2)
{
    CRW_Route *R = CRW_route_new();
    int err = CRW_route_init(R, "/a");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected");
    CRW_route_tag_dump(R, "regex_clean2");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_clean3)
{
    CRW_Route *R = CRW_route_new();
    int err = CRW_route_init(R, "/hello");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected");
    CRW_route_tag_dump(R, "regex_clean3");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_simple1)
{
    CRW_Route *R = CRW_route_new();
    int err = CRW_route_init(R, "/hello/:name");
    int num = 0;
    const char *val = NULL;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 1, "found unexpected number of tags: %i", num);
    val = CRW_route_tag_get_by_idx(R, 0);
    fail_if(strcmp(val, "name") != 0, "tag has unexpected value: [%s]", val);
    CRW_route_tag_dump(R, "regex_simple1");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_simple2)
{
    CRW_Route *R = CRW_route_new();
    int err = CRW_route_init(R, "/hello/:name/:surname");
    int num = 0;
    const char *val = NULL;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 2, "found unexpected number of tags: %i", num);
    val = CRW_route_tag_get_by_idx(R, 0);
    fail_if(strcmp(val, "name") != 0, "tag has unexpected value: [%s]", val);
    val = CRW_route_tag_get_by_idx(R, 1);
    fail_if(strcmp(val, "surname") != 0, "tag has unexpected value: [%s]", val);
    CRW_route_tag_dump(R, "regex_simple2");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_simple3)
{
    CRW_Route *R = CRW_route_new();
    int err = CRW_route_init(R, "/hello/:name/:surname/aka/:nickname");
    int num = 0;
    const char *val = NULL;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 3, "found unexpected number of tags: %i", num);
    val = CRW_route_tag_get_by_idx(R, 0);
    fail_if(strcmp(val, "name") != 0, "tag has unexpected value: [%s]", val);
    val = CRW_route_tag_get_by_idx(R, 1);
    fail_if(strcmp(val, "surname") != 0, "tag has unexpected value: [%s]", val);
    val = CRW_route_tag_get_by_idx(R, 2);
    fail_if(strcmp(val, "nickname") != 0, "tag has unexpected value: [%s]", val);
    CRW_route_tag_dump(R, "regex_simple3");
    CRW_route_del(R);
}
END_TEST


/*************************************************************************/

TCase *craneweb_testCaseRoute(void)
{
    TCase *tcRoute = tcase_create("craneweb.core.route");
    tcase_add_test(tcRoute, test_none);
    tcase_add_test(tcRoute, test_regex_empty);
    tcase_add_test(tcRoute, test_regex_clean1);
    tcase_add_test(tcRoute, test_regex_clean2);
    tcase_add_test(tcRoute, test_regex_clean3);
    tcase_add_test(tcRoute, test_regex_simple1);
    tcase_add_test(tcRoute, test_regex_simple2);
    tcase_add_test(tcRoute, test_regex_simple3);
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

