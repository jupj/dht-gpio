#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include <unistd.h>

#include "convert.h"

// cmd_usage prints usage on stderr
int cmd_usage(int argc, char **args) {
    fprintf(stderr, "%s blinks a led\n", args[0]);
    fprintf(stderr, "Usage: %s GPIOCHIP OUTPORT [ INPORT ]\n", args[0]);
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "    # Blink led on port PC12\n");
    fprintf(stderr, "    %s gpiochip1 PC12\n", args[0]);
    fprintf(stderr, "    # Blink led on port PC12 and read led status on port PC7\n");
    fprintf(stderr, "    %s gpiochip1 PC12 PC7\n", args[0]);
    return 0;
}

int cmd_blink(int argc, char **args) {
    if (argc < 3 || argc > 4) {
        cmd_usage(argc, args);
        return 1;
    }

    char *chipname = args[1];
    char *outport = args[2];
    char *inport = NULL;
    if (argc > 3) {
        inport = args[3];
    }
    int outport_nr = convert(outport);
    if (outport_nr == CONVERT_INVALID_PORT) {
        fprintf(stderr, "%s is not a valid port name\n", outport);
        return 1;
    }

    int inport_nr = -1;
    if (inport != NULL) {
        inport_nr = convert(inport);
        if (inport_nr == CONVERT_INVALID_PORT) {
            fprintf(stderr, "%s is not a valid port name\n", inport);
            return 1;
        }
    }

    int rc = 0;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line_out = NULL;
    struct gpiod_line *line_in = NULL;
    const char *consumer = "blink";

    chip = gpiod_chip_open_by_name(chipname);
    if (chip == NULL) {
        perror(chipname);
        rc = 1;
        goto cleanup;
    }

    line_out = gpiod_chip_get_line(chip, outport_nr);
    if (line_out == NULL) {
        perror(outport);
        rc = 1;
        goto cleanup;
    }
    if (gpiod_line_request_output(line_out, consumer, 0)) {
        perror("requested output");
        rc = 1;
        goto cleanup;
    }

    if (inport_nr > -1) {
        line_in = gpiod_chip_get_line(chip, inport_nr);
        if (line_in == NULL) {
            perror(inport);
            rc = 1;
            goto cleanup;
        }
        if (gpiod_line_request_input(line_in, consumer)) {
            perror("requested input");
            rc = 1;
            goto cleanup;
        }
        //if (gpiod_line_is_used(line_in)) {
            //printf("inport %d is used\n", inport_nr);
        //}
        //if (gpiod_line_request_both_edges_events(line_in, consumer)) {
            //perror("request input events");
            //rc = 1;
            //goto cleanup;
        //}

        //struct gpiod_line_request_config config = {
            //.consumer = consumer,
            //.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES,
            //.flags = 0,
        //};
        //if (gpiod_line_request(line_in, &config, 0)) {
            //perror("request input with config");
            //rc = 1;
            //goto cleanup;
        //}
    }

    // Blink the led 2 seconds
    if (gpiod_line_set_value(line_out, 1)) {
        perror("set val");
        rc = 1;
        goto cleanup;
    }
    printf("Led ON\n");

    if (line_in != NULL) {
        int val = gpiod_line_get_value(line_in);
        if (val < 0) {
            perror("gpiod_line_get_value");
            rc = 1;
            goto cleanup;
        }
        printf("%s = %d\n", inport, val);
    }

    sleep(2);

    if (gpiod_line_set_value(line_out, 0)) {
        perror("set val");
        rc = 1;
        goto cleanup;
    }
    printf("Led OFF\n");

    if (line_in != NULL) {
        int val = gpiod_line_get_value(line_in);
        if (val < 0) {
            perror("gpiod_line_get_value");
            rc = 1;
            goto cleanup;
        }
        printf("%s = %d\n", inport, val);
    }

cleanup:
    if (line_out != NULL) {
        gpiod_line_release(line_out);
    }
    if (line_in != NULL) {
        gpiod_line_release(line_in);
    }
    if (chip != NULL) {
        gpiod_chip_close(chip);
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
