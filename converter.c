#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "convert.h"

// cmd_usage prints usage on stderr
int cmd_usage(int argc, char **args) {
    fprintf(stderr, "%s converts PINE A64 Pi2 connector port names to the gpio number\n", args[0]);
    fprintf(stderr, "Usage: %s <PORT>\n", args[0]);
    fprintf(stderr, "Example: %s PC13\n", args[0]);
    return 0;
}

int cmd_convert(int argc, char **args) {
    // convert PORT name to GPIO number
    int gpionum = convert(args[1]);
    if (gpionum == CONVERT_INVALID_PORT) {
        fprintf(stderr, "%s is not a valid port name\n", args[1]);
        return 1;
    }

    printf("%d\n", gpionum);
    return 0;
}

struct cmd {
    char *cmd;
    int (*fn)(int argc, char **args);
} cmds[] = {
    {"-h", cmd_usage},
    {"--help", cmd_usage},
    {"help", cmd_usage},
    // default command:
    {NULL, cmd_convert},
};

#define N_CMDS (sizeof(cmds)/sizeof(cmds[0]))

int main(int argc, char **args) {
    if (argc < 2) {
        cmd_usage(argc, args);
        return 1;
    }

    for (int i=0; i<N_CMDS; i++) {
        if (cmds[i].cmd != NULL && strcmp(cmds[i].cmd, args[1]) == 0) {
            return cmds[i].fn(argc, args);
        }
    }

    // Run default command
    return cmds[N_CMDS-1].fn(argc, args);
}
