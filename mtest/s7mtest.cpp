#include <cstdlib>
#include <cstdio>
#include "s7/s7.h"
#include "s7/utils.h"

#include <QtTest/QtTest>
#include "libmscore/score.h"
#include "libmscore/element.h"
#include "libmscore/chord.h"
#include "libmscore/tremolo.h"
#include "mtest/testutils.h"
using namespace Ms;
#include "s7gen.h"


extern Ms::MTest* g_mtest;
extern int g_test_check_pass;
extern int g_test_check_fail;

s7_pointer ms_test_check (s7_scheme *sc, s7_pointer args)
{
    int good = 1;
    s7_pointer s = s7_car(args);
    if (s7_is_boolean(s)) {
        if (s7_boolean(sc, s)) {
        } else {
            g_test_check_fail++;
            good = 0;
        }
    } else if (s7_is_null(sc, s)) {
        g_test_check_fail++;
        good = 0;
    }
    if (good) {
      g_test_check_pass++;
      return s7_t(sc);
    }
    return s7_nil(sc);
}

s7_pointer ms_test_check_pass (s7_scheme *sc, s7_pointer args)
{
    (void) args;
    g_test_check_pass++;
    return s7_nil(sc);
}

s7_pointer ms_test_check_fail (s7_scheme *sc, s7_pointer args)
{
    (void) args;
    g_test_check_fail++;
    return s7_nil(sc);
}

const char* my_s7_get_car_as_string (s7_pointer lst)
{
    if (s7_is_pair(lst) &&
        s7_is_string(s7_car(lst))) {
        return s7_string(s7_car(lst));
    } else {
        return NULL;
    }
}

s7_pointer ms_mtest_readScore (s7_scheme *sc, s7_pointer args)
{
    const char *s = my_s7_get_car_as_string(args);
    if (! s) return s7_f(sc);
    MasterScore* score = g_mtest->readScore(QString(s));
    uint64_t ty = 0;
    return c_make_goo(sc, ty, s7_nil(sc), score);
}

s7_pointer ms_mtest_saveCompareScore (s7_scheme *sc, s7_pointer args)
{
    if (!s7_is_pair(args)) return s7_f(sc);
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    goo_t *g = (goo_t *)s7_c_object_value(s);
    MasterScore* score = (MasterScore *) g->cd;
    args = s7_cdr(args);
    const char *w_file = my_s7_get_car_as_string(args);
    if (! w_file) return s7_f(sc);
    args = s7_cdr(args);
    const char *ref_file = my_s7_get_car_as_string(args);
    if (! ref_file) return s7_f(sc);

    if (g_mtest->saveCompareScore(score, QString(w_file), QString(ref_file))) {
        return s7_t(sc);
    }
    return s7_f(sc);
}

s7_pointer ms_mtest_writeReadElement(s7_scheme *sc, s7_pointer args)
{
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) {
        return s7_nil(sc);
    }
    goo_t *g = (goo_t *)s7_c_object_value(s);
    Element* e = (Element*) g->cd;
    return c_make_goo(sc, g->ty, g->data, g_mtest->writeReadElement(e));
}


void mtest_s7_define_functions(s7_scheme *sc) {
    init_goo(sc);
    init_gen_s7(sc);
    s7_define_function(sc, "ms-test-check", ms_test_check, 1, 0, false, "(ms-test-check cond)");
    s7_define_function(sc, "ms-test-check-pass", ms_test_check_pass, 0, 0, false, "(ms-test-check-pass)");
    s7_define_function(sc, "ms-test-check-fail", ms_test_check_fail, 0, 0, false, "(ms-test-check-fail)");
    s7_define_function(sc, "ms-mtest-writeReadElement", ms_mtest_writeReadElement, 1, 0, false, "(ms-mtest-writeReadElement Element)");
    s7_define_function(sc, "ms-mtest-saveCompareScore", ms_mtest_saveCompareScore, 3, 0, false, "(ms-mtest-saveCompareScore Score WriteFile RefFile)");
    s7_define_function(sc, "ms-mtest-readScore", ms_mtest_readScore, 1, 0, false, "(ms-mtest-readScore <score-filename>)");
}

int run_scheme_script(const char *filename)
{
    s7_scheme *s7;
    s7 = s7_init();
    mtest_s7_define_functions(s7);
    if (!s7_load(s7, filename)) {
        return -1;
    }
    free(s7);
    return 0;
}
