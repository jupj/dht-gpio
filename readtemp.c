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

/*** API for DHT sensor ***/

#define RET_ON_ERR(result) if (result < 0) { perror(#result); return result; }

// read_dht_sensor reads the dht sensor
// returns: 0 if successful, -1 if error
int read_dht_sensor(struct gpiod_chip *chip, struct gpiod_line *line, const char *consumer) {
    struct timespec ts, start;
    const int expected_bits = 40;
    const int N = expected_bits*2;
    struct timespec log[N];
    int val;

    // Write to activate the DHT sensor
    RET_ON_ERR(gpiod_line_request_output(line, consumer, 1));
    RET_ON_ERR(clock_gettime(CLOCK_MONOTONIC, &ts));
    RET_ON_ERR(gpiod_line_set_value(line, 0));
    // Sleep 19ms
    add(&ts, 19000);
    RET_ON_ERR(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL));

    // Pull up and start polling input from line
    val = 1;
    RET_ON_ERR(gpiod_line_set_value(line, 1));
    RET_ON_ERR(clock_gettime(CLOCK_MONOTONIC, &ts));

    gpiod_line_release(line);
    RET_ON_ERR(gpiod_line_request_input(line, consumer));

    start = ts;
    RET_ON_ERR(clock_gettime(CLOCK_MONOTONIC, &ts));

    // Poll falling edges
    const int interval_us = 15;
    int samecnt = 0, timeout = 2000/interval_us;
    int len = 0;
    while (len < N && samecnt < timeout) {
        int newval = gpiod_line_get_value(line);
        if (newval < 0) {
            perror("gpiod_line_get_value");
            return -1;
        }
        if (newval < val) {
            // Falling edge;
            RET_ON_ERR(clock_gettime(CLOCK_MONOTONIC, &log[len]));
            len++;
            samecnt = 0;
        }
        val = newval;
        samecnt++;
        add(&ts, interval_us);
        RET_ON_ERR(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL));
    }

    if (len <= expected_bits) {
        fprintf(stderr, "Fail: read only %d falling edges, need %d\n", len, expected_bits + 1);
        return -1;
    }

    const int start_ix = len - expected_bits;
    // Print logged falling edges
    if (0) {
        for (int i=0; i<len; i++) {
            if (i == start_ix) {
                printf("*** START ***\n");
            }
            printf("Log[%d]: %ld us", i, diff(&start, &log[i]));
            if (i > 0) {
                printf(" / +%ldus", diff(&log[i-1], &log[i]));
            }
            printf("\n");
        }
    }

    // Decode the bits into relative humidity (%) and temperature (celsius)
    int hum_int = 0;
    int hum_dec = 0;
    int temp_int = 0;
    int temp_dec = 0;
    int checksum = 0;
    for (int i=0; i<8; i++) {
        hum_int <<= 1;
        int delta_us = diff(&log[start_ix+i-1], &log[start_ix+i]);
        if (delta_us > 100) {
            hum_int += 1;
        }

        hum_dec <<= 1;
        delta_us = diff(&log[start_ix+8+i-1], &log[start_ix+8+i]);
        if (delta_us > 100) {
            hum_dec += 1;
        }

        temp_int <<= 1;
        delta_us = diff(&log[start_ix+16+i-1], &log[start_ix+16+i]);
        if (delta_us > 100) {
            temp_int += 1;
        }

        temp_dec <<= 1;
        delta_us = diff(&log[start_ix+24+i-1], &log[start_ix+24+i]);
        if (delta_us > 100) {
            temp_dec += 1;
        }

        checksum <<= 1;
        delta_us = diff(&log[start_ix+32+i-1], &log[start_ix+32+i]);
        if (delta_us > 100) {
            checksum += 1;
        }
    }

    int checksum_got = hum_int + hum_dec + temp_int + temp_dec;
    if (checksum != checksum_got) {
        printf("FAIL: checksum %d doesn't match packet's checksum %d\n", checksum_got, checksum);
        return -1;
    }

    printf("RH: %d.%.2d%% / T: %d.%.2d C / CHK: %d\n", hum_int, 100*hum_dec/256, temp_int, 100*temp_dec/256, checksum);
    return 0;
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
    char *chipname = "gpiochip1";
    char *port = "PC12";

    if (argc == 3) {
        chipname = args[1];
        port = args[2];
    } else if (argc != 1) {
        cmd_usage(argc, args);
        return 1;
    }

    // Set Real-Time behaviour
    set_rt_sched();
    no_swapping();

    int port_nr = convert(port);
    if (port_nr == CONVERT_INVALID_PORT) {
        fprintf(stderr, "%s is not a valid port name\n", port);
        return 1;
    }

    int rc = 0;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line = NULL;
    const char *consumer = args[0];

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

    if (read_dht_sensor(chip, line, consumer)) {
        rc = 1;
    }

cleanup:
    if (line != NULL) {
        gpiod_line_release(line);
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
    {NULL, cmd_activate},
};

#define N_CMDS (sizeof(cmds)/sizeof(cmds[0]))

int main(int argc, char **args) {
    if (argc > 1) {
        for (int i=0; i<N_CMDS; i++) {
            if (cmds[i].cmd != NULL && strcmp(cmds[i].cmd, args[1]) == 0) {
                return cmds[i].fn(argc, args);
            }
        }
    }

    // Run default command
    return cmds[N_CMDS-1].fn(argc, args);
}
