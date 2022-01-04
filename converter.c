#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// References:
// * http://forum.pine64.org/showthread.php?tid=474
// * https://gist.github.com/longsleep/6ab75289bf92cbe3d02a0c5d3f6a4764

#define INVALID_PORT -1

// convert takes a port name, such as PC13 and converts it into the
// corresponding GPIO number for the Pine A64+
int convert(char *port) {
    if (strlen(port) <3) {
        return INVALID_PORT;
    }

    if (port[0] != 'P') {
        return INVALID_PORT;
    }

    if (!isupper(port[1])) {
        return INVALID_PORT;
    }

    for (int i=2; i<strlen(port); i++) {
        if (!isdigit(port[i])) {
            return INVALID_PORT;
        }
    }

    int bank = port[1] - 'A';
    int num = atoi(&port[2]);

    return 32*bank + num;
}

// usage prints usage on stderr
int usage(int argc, char **args) {
    fprintf(stderr, "%s converts PINE A64 Pi2 connector port names to the gpio number\n", args[0]);
    fprintf(stderr, "Usage: %s <PORT>\n", args[0]);
    fprintf(stderr, "Example: %s PC13\n", args[0]);
    return 0;
}

// unit_test runs tests on convert() and returns number of failed cases.
// Prints results on stderr
int unit_test(int argc, char **args) {
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

struct cmd {
    char *cmd;
    int (*fn)(int argc, char **args);
} cmds[] = {
    {"-h", usage},
    {"--help", usage},
    {"help", usage},
    {"test", unit_test},
};

#define N_CMDS (sizeof(cmds)/sizeof(cmds[0]))

int main(int argc, char **args) {
    if (argc < 2) {
        usage(argc, args);
        return 1;
    }

    for (int i=0; i<N_CMDS; i++) {
        if (strcmp(cmds[i].cmd, args[1]) == 0) {
            return cmds[i].fn(argc, args);
        }
    }

    // convert PORT name to GPIO number
    int gpionum = convert(args[1]);
    if (gpionum == INVALID_PORT) {
        fprintf(stderr, "%s is not a valid port name\n", args[1]);
        return 1;
    }

    printf("%d\n", gpionum);
    return 0;
}
