#include <stdio.h>
#include <stdlib.h>
#include "convert.h"

/***
 * Unit tests
 */

// convert_test runs tests on convert() and returns number of failed cases.
// Prints results on stderr
int convert_test() {
    struct {
        char *port;
        int expected;
    } testcases[] = {
        {"PC13", 77},
        {"PB0", 32},
        {"PH7", 231},
        {"PL8", 360},
    };
    int n = sizeof(testcases)/sizeof(testcases[0]);

    int failcnt = 0;
    for (int i=0; i<n; i++) {
        int got = convert(testcases[i].port);
        if (got != testcases[i].expected) {
            fprintf(stderr, "convert(\"%s\"): got %d, expected %d\n", testcases[i].port, got, testcases[i].expected);
            failcnt++;
        }
    }
    if (failcnt == 0) {
        fprintf(stderr, "PASS: all %d tests passed\n", n);
    } else {
        fprintf(stderr, "FAIL: %d of %d tests failed\n", failcnt, n);
    }
    return failcnt;
}

int main(int argc, char **args) {
    int failed = convert_test();
    if (failed > 0) {
        exit(EXIT_FAILURE);
    }
    return 0;
}
