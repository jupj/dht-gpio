#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <errno.h>

#include "convert.h"

/*** API for struct timespec ***/
void add(struct timespec *ts, int usec) {
    ts->tv_nsec += 1000*usec;
    while (ts->tv_nsec > 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}

// diff returns the number of microseconds (us) between timespecs.
long diff(const struct timespec *t1, const struct timespec *t2) {
    long res = t2->tv_nsec - t1->tv_nsec;
    if (t2->tv_sec > t1->tv_sec) {
        res += 1000000000*(t2->tv_sec - t1->tv_sec);
    }
    return res/1000;
}

/*** API for Real-Time behaviour ***/
void set_rt_sched() {
    // Set thread to real time priority
    struct sched_param sp;
    sp.sched_priority = 1;
    if (sched_setscheduler(0, SCHED_FIFO, &sp)) {
        perror("Failed to set thread to real-time priority");
    }

    // If using pthreads, set real time priority for the current thread instead:
    // pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
}

void no_swapping() {
    // Lock memory to ensure no swapping is done
    if (mlockall(MCL_FUTURE|MCL_CURRENT)) {
        perror("Failed to lock memory from swapping");
    }
}


// cmd_usage prints usage on stderr
int cmd_usage(int argc, char **args) {
    fprintf(stderr, "%s activates a DHT11 sensor\n", args[0]);
    fprintf(stderr, "Usage: %s GPIOCHIP PORT\n", args[0]);
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "    # Activate DHT11 sensor on port PC12\n");
    fprintf(stderr, "    %s gpiochip1 PC12\n", args[0]);
    return 0;
}

#define CHECK(result) if (result) { perror(#result); rc = 1; goto cleanup; }

int cmd_activate(int argc, char **args) {
    printf("Line %d\n", __LINE__);
    char *chipname = "gpiochip1";
    char *port = "PC12";

    if (argc == 3) {
        chipname = args[1];
        port = args[2];
    } else if (argc != 1) {
        cmd_usage(argc, args);
        return 1;
    }

    printf("Line %d\n", __LINE__);
    // Set Real-Time behaviour
    set_rt_sched();
    no_swapping();

    printf("Line %d\n", __LINE__);
    int port_nr = convert(port);
    if (port_nr == CONVERT_INVALID_PORT) {
        fprintf(stderr, "%s is not a valid port name\n", port);
        return 1;
    }

    int rc = 0;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line = NULL;
    const char *consumer = args[0];

    struct timespec ts, start;
    const int N = 400;
    struct {
        struct timespec ts;
        int val;
    } log[N];

    chip = gpiod_chip_open_by_name(chipname);
    if (chip == NULL) {
        perror(chipname);
        rc = 1;
        goto cleanup;
    }

    line = gpiod_chip_get_line(chip, port_nr);
    if (line == NULL) {
        perror(port);
        rc = 1;
        goto cleanup;
    }

    // Write to activate the DHT sensor
    printf("Activating sensor...\n");
    CHECK(gpiod_line_request_output(line, consumer, 1));
    CHECK(clock_gettime(CLOCK_MONOTONIC, &ts));
    CHECK(gpiod_line_set_value(line, 0));
    // Sleep 19ms
    add(&ts, 19000);
    CHECK(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL));

    // Pull up and start polling input from line
    CHECK(gpiod_line_set_value(line, 1));
    CHECK(clock_gettime(CLOCK_MONOTONIC, &ts));
    gpiod_line_release(line);
    CHECK(gpiod_line_request_input(line, consumer));

    start = ts;
    for (int i=0; i<N; i++) {
        log[i].val = gpiod_line_get_value(line);
        CHECK(clock_gettime(CLOCK_MONOTONIC, &log[i].ts));

        //ts = log[i].ts;
        add(&ts, 15);
        CHECK(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL));
    }

cleanup:
    printf("Cleanup\n");
    if (line != NULL) {
        gpiod_line_release(line);
    }
    if (chip != NULL) {
        gpiod_chip_close(chip);
    }

    for (int i=1; i<N; i++) {
        printf("Log[%d]: PIN=%d\t %ld us\n", i, log[i].val, diff(&start, &log[i].ts));
    }

    int val = log[0].val;
    int edge = 0;
    int bits = 0;
    for (int i=1; i<N; i++) {
        // Timeout if more than 1ms
        if (diff(&log[edge].ts, &log[i].ts) > 1000) {
            printf("timeout\n");
            break;
        }
        if (log[i].val != val || i == (N-1)) {
            // Edge: print timing info
            long in_intv = diff(&log[edge].ts, &log[i-1].ts);
            long out_intv = diff(&log[edge].ts, &log[i].ts);
            if (edge > 0) {
                out_intv = diff(&log[edge-1].ts, &log[i].ts);
            }
            printf("PIN=%d    %4ldus - %4ldus | %3ld-%ldus", val, diff(&start, &log[edge].ts), diff(&start, &log[i-1].ts), in_intv, out_intv);
            if (val == 1) {
                bits++;
                if (in_intv <= 28 && out_intv <= 65) {
                    printf(" => '0'");
                } else if (in_intv > 28 && out_intv > 50) {
                    printf(" => '1'");
                } else {
                    printf(" => '?'");
                }
            }
            printf("\n");
            // Init new value
            val = log[i].val;
            edge = i;
        }
        //printf("Log[%d]: PIN=%d\t %ld us\n", i, log[i].val, diff(&start, &log[i].ts));
    }
    printf("%d bits\n", bits);

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
    {NULL, cmd_activate},
};

#define N_CMDS (sizeof(cmds)/sizeof(cmds[0]))

int main(int argc, char **args) {
    printf("Line %d\n", __LINE__);
    if (argc > 1) {
        for (int i=0; i<N_CMDS; i++) {
            printf("Line %d\n", __LINE__);
            if (cmds[i].cmd != NULL && strcmp(cmds[i].cmd, args[1]) == 0) {
                printf("Line %d\n", __LINE__);
                return cmds[i].fn(argc, args);
            }
        }
    }

    printf("Line %d\n", __LINE__);
    // Run default command
    return cmds[N_CMDS-1].fn(argc, args);
}
