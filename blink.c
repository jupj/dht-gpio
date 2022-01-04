#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include <unistd.h>

#include "convert.h"

// cmd_usage prints usage on stderr
int cmd_usage(int argc, char **args) {
    fprintf(stderr, "%s blinks a led\n", args[0]);
    fprintf(stderr, "Usage: %s <gpiochip> <port>\n", args[0]);
    fprintf(stderr, "Example: %s gpiochip1 PC12\n", args[0]);
    return 0;
}

int cmd_blink(int argc, char **args) {
    if (argc != 3) {
        cmd_usage(argc, args);
        return 1;
    }

    char *chipname = args[1];
    char *portname = args[2];
    int portline = convert(portname);
    if (portline == CONVERT_INVALID_PORT) {
        fprintf(stderr, "%s is not a valid port name\n", portname);
        return 1;
    }

    int rc = 0;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line = NULL;

    chip = gpiod_chip_open_by_name(chipname);
    if (chip == NULL) {
        perror(chipname);
        rc = 1;
        goto cleanup;
    }

    line = gpiod_chip_get_line(chip, portline);
    if (line == NULL) {
        perror(portname);
        rc = 1;
        goto cleanup;
    }
    const char *consumer = "blink";
    if (gpiod_line_request_output(line, consumer, 0)) {
        perror("requested output");
        rc = 1;
        goto cleanup;
    }

    // Blink the led 2 seconds
    if (gpiod_line_set_value(line, 1)) {
        perror("set val");
        rc = 1;
        goto cleanup;
    }
    printf("Led ON\n");

    sleep(2);

    if (gpiod_line_set_value(line, 0)) {
        perror("set val");
        rc = 1;
        goto cleanup;
    }
    printf("Led OFF\n");

cleanup:
    if (chip != NULL) {
        gpiod_chip_close(chip);
    }
    if (line != NULL) {
        gpiod_line_release(line);
    }

    return rc;
}

struct cmd {
    char *cmd;
    int (*fn)(int argc, char **args);
} cmds[] = {
    {"-h", cmd_usage},
    {"--help", cmd_usage},
    {"help", cmd_usage},
    // default command:
    {NULL, cmd_blink},
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
