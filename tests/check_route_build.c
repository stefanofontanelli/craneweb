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

static int init_route(CRW_Route *R, const char *str,
                      const char *tags[], int num)
{
    int err = 0;
    if (R && str) {
        err = CRW_route_setup(R, str);
        if (!err) {
            CRW_route_set_tags(R, tags, num);
            err = CRW_route_build_crane_regex(R);
        }
    }
    return err;
}

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
    int err = init_route(R, "", NULL, 0);
    fail_if(err, "route init failed");
    CRW_route_del(R);
}
END_TEST

static void tester_regex_clean(const char *test_name, const char *regex)
{
    const char *re_u = NULL, *re_c = NULL;
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, regex, NULL, 0);
    fail_if(err, "route init failed");
    CRW_route_regex_dump(R, test_name);
    re_u = CRW_route_regex_user(R);
    re_c = CRW_route_regex_crane(R);
    fail_if(strcmp(re_u, re_c),
            "unexpected regex differences: [%s]->[%s]",
            re_u, re_c);
    CRW_route_del(R);
    return;
}

START_TEST(test_regex_clean1)
{
    tester_regex_clean("test_regex_clean1", "/");
}
END_TEST

START_TEST(test_regex_clean2)
{
    tester_regex_clean("test_regex_clean2", "/a");
}
END_TEST

START_TEST(test_regex_clean3)
{
    tester_regex_clean("test_regex_clean3", "/hello");
}
END_TEST

static void tester_regex_simple(const char *test_name, const char *regex,
                                const char *tags[], int num)
{
    const char *re_u = NULL, *re_c = NULL;
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, regex, tags, num);
    fail_if(err, "route init failed");
    CRW_route_regex_dump(R, test_name);
    re_u = CRW_route_regex_user(R);
    re_c = CRW_route_regex_crane(R);
    fail_unless(strcmp(re_u, re_c),
                "expected regex differences: [%s]->[%s]",
                re_u, re_c);
    CRW_route_del(R); 
}

START_TEST(test_regex_simple1)
{
    const char *tags[] = { "name", NULL };
    tester_regex_simple("test_regex_simple1",
                        "/hello/:name",
                        tags, 1);
}
END_TEST

START_TEST(test_regex_simple2)
{
    const char *tags[] = { "name", "surname", NULL };
    tester_regex_simple("test_regex_simple2",
                        "/hello/:name/:surname",
                        tags, 2);
}
END_TEST

START_TEST(test_regex_simple3)
{
    const char *tags[] = { "name", "surname", "nickname", NULL };
    tester_regex_simple("test_regex_simple3",
                        "/hello/:name/:surname/aka/:nickname",
                        tags, 3);
}
END_TEST

#if 0
START_TEST(test_regex_allslashes2)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, "//");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected");
    CRW_route_tag_dump(R, "allslashes2");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_allslashes3)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, "///");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected");
    CRW_route_tag_dump(R, "allslashes3");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_emptytag1)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, ":");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected: %i", num);
    CRW_route_tag_dump(R, "emptytag1");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    fail_unless(CRW_route_tag_malformed(R) == 1, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_emptytag2)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, "::");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected: %i", num);
    CRW_route_tag_dump(R, "emptytag2");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    fail_unless(CRW_route_tag_malformed(R) == 2, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_emptytag3)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, ":::");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected: %i", num);
    CRW_route_tag_dump(R, "emptytag3");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    fail_unless(CRW_route_tag_malformed(R) == 3, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_emptytag4)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, "/:::");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected: %i", num);
    CRW_route_tag_dump(R, "emptytag4");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    fail_unless(CRW_route_tag_malformed(R) == 3, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_emptytag5)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, ":/::");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected: %i", num);
    CRW_route_tag_dump(R, "emptytag5");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    fail_unless(CRW_route_tag_malformed(R) == 3, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_emptytag6)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, "::/:");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected: %i", num);
    CRW_route_tag_dump(R, "emptytag6");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    fail_unless(CRW_route_tag_malformed(R) == 3, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_emptytag7)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, ":::/");
    int num = 0;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 0, "found tags wherever unexpected: %i", num);
    CRW_route_tag_dump(R, "emptytag7");
    fail_unless(CRW_route_all_empty_tags(R), "found the unexpected");
    fail_unless(CRW_route_tag_malformed(R) == 3, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_tagnoise1)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, ":a::");
    int num = 0;
    const char *val = NULL;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 1, "miscounted tags: %i", num);
    val = CRW_route_tag_get_by_idx(R, 0);
    fail_if(strcmp(val, "a") != 0, "tag has unexpected value: [%s]", val);
    CRW_route_tag_dump(R, "tagnoise1");
    fail_unless(CRW_route_tag_malformed(R) == 2, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST

START_TEST(test_regex_tagnoise2)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, "::a:");
    int num = 0;
    const char *val = NULL;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 1, "miscounted tags: %i", num);
    val = CRW_route_tag_get_by_idx(R, 0);
    fail_if(strcmp(val, "a") != 0, "tag has unexpected value: [%s]", val);
    CRW_route_tag_dump(R, "tagnoise1");
    fail_unless(CRW_route_tag_malformed(R) == 2, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST
START_TEST(test_regex_tagnoise3)
{
    CRW_Route *R = CRW_route_new();
    int err = init_route(R, ":::a");
    int num = 0;
    const char *val = NULL;
    fail_if(err, "route init failed");
    num = CRW_route_tag_count(R);
    fail_unless(num == 1, "miscounted tags: %i", num);
    val = CRW_route_tag_get_by_idx(R, 0);
    fail_if(strcmp(val, "a") != 0, "tag has unexpected value: [%s]", val);
    CRW_route_tag_dump(R, "tagnoise1");
    fail_unless(CRW_route_tag_malformed(R) == 2, "malformed tag miscount");
    CRW_route_del(R);
}
END_TEST
#endif

/*************************************************************************/

TCase *craneweb_testCaseRoute(void)
{
    TCase *tcRoute = tcase_create("craneweb.core.route.build");
    tcase_add_test(tcRoute, test_none);
    tcase_add_test(tcRoute, test_regex_empty);
    tcase_add_test(tcRoute, test_regex_clean1);
    tcase_add_test(tcRoute, test_regex_clean2);
    tcase_add_test(tcRoute, test_regex_clean3);
    tcase_add_test(tcRoute, test_regex_simple1);
    tcase_add_test(tcRoute, test_regex_simple2);
    tcase_add_test(tcRoute, test_regex_simple3);
#if 0
    tcase_add_test(tcRoute, test_regex_allslashes2);
    tcase_add_test(tcRoute, test_regex_allslashes3);
    tcase_add_test(tcRoute, test_regex_emptytag1);
    tcase_add_test(tcRoute, test_regex_emptytag2);
    tcase_add_test(tcRoute, test_regex_emptytag3);
    tcase_add_test(tcRoute, test_regex_emptytag4);
    tcase_add_test(tcRoute, test_regex_emptytag5);
    tcase_add_test(tcRoute, test_regex_emptytag6);
    tcase_add_test(tcRoute, test_regex_emptytag7);
    tcase_add_test(tcRoute, test_regex_tagnoise1);
    tcase_add_test(tcRoute, test_regex_tagnoise2);
    tcase_add_test(tcRoute, test_regex_tagnoise3);
#endif
    return tcRoute;
}

static Suite *craneweb_suiteRoute(void)
{
    TCase *tc = craneweb_testCaseRoute();
    Suite *s = suite_create("craneweb.core.route.build");
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

