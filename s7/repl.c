#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "s7/s7.h"
#include "utils.h"

int run_scheme_eval_buffer (s7_scheme *sc, char *buffer)
{
    if (sc && buffer)
        s7_eval_c_string(sc, buffer); /* evaluate input and write the result */
    return 0;
}

s7_pointer main_make_scheme_arguments (s7_scheme *sc, int argc, char **argv) {
    s7_pointer lst = s7_nil(sc);
    for (int i = 1; i < argc; i++) {
        lst = s7_cons(sc, s7_nil(sc), lst);
        s7_set_car(lst, s7_make_string(sc, argv[i]));
    }
    return s7_reverse(sc, lst);
}

int main (int argc, char **argv) {
    int f_read_stdin = 1;
    s7_scheme *s7 = s7_init();
    // make a scheme-list of c-arguments
    s7_pointer lst = main_make_scheme_arguments(s7, argc, argv);
    s7_define_variable(s7, "%command-line", lst);
    // process c-arguments
    for (int i = 1; i < argc; i++) {
        if (!(strcmp("--", argv[i]))) {
            break; // -- marks that rest of arguments are given to scheme
        } else if (!(strcmp("-f", argv[i]))) {
            f_read_stdin = 0; // file-only mode
        } else if (!s7_load(s7, argv[i])) {
            fprintf(stderr, "can't load %s\n", argv[i]);
        }
    }
    if (f_read_stdin) {
        run_scheme_eval_buffer(s7, slurp_stream(stdin, 1));
    }
    free(s7);
    return 0;
}

