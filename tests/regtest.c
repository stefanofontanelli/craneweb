#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "regex.h"

enum {
    MATCHES = 16
};

static void mark(const char *str, int n, int s, int e)
{
    int j;
    printf("%02i: %s\n", n, str);
    putchar(' '); putchar(' '); putchar(' '); putchar(' ');
    for (j = 0; str && str[j]; j++) {
        if (j < s || j >= e) {
            putchar('_');
        } else {
            putchar('^');
        }
    }
    putchar('\n');
    return;
}

int main(int argc, char *argv[])
{
    regex_t RE;
    int err;

    if (argc != 3) {
        return 1;
    }

    err = regcomp(&RE, argv[1], REG_EXTENDED);
    if (!err) {
        regmatch_t MT[MATCHES];
        err = regexec(&RE, argv[2], MATCHES, MT, 0);
        if (!err) {
            int j = 0;
            mark(argv[2], j, MT[j].rm_so, MT[j].rm_eo);
            for (j = 1; MT[j].rm_so != -1 && MT[j].rm_eo != -1; j++) {
                mark(argv[2], j, MT[j].rm_so, MT[j].rm_eo);
            }
        }
    }

    return err;
}

