#include <stdio.h>
#include <time.h>
#include <sys/mman.h>
#include <gpiod.h>
//#include <pthread.h>
#include <sched.h>

#include "convert.h"

#define CHECK(res) if (res) { perror(#res); return 1; }

long diff(const struct timespec *t1, const struct timespec *t2) {
    long res = t2->tv_nsec - t1->tv_nsec;
    if (t2->tv_sec > t1->tv_sec) {
        res += 1000000000*(t2->tv_sec - t1->tv_sec);
    }
    return res;
}

void add(struct timespec *ts, int usec) {
    ts->tv_nsec += 1000*usec;
    while (ts->tv_nsec > 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}

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

struct gpio_pin {
    struct gpiod_chip *chip;
    struct gpiod_line *line;
};

int gpio_init(struct gpio_pin *pin, char *chipname, char *portname) {
    pin->chip = NULL;
    pin->line = NULL;
    const char *consumer = "rtlog";

    int port_nr = convert(portname);
    if (port_nr == CONVERT_INVALID_PORT) {
        fprintf(stderr, "%s is not a valid port name\n", portname);
        return -1;
    }

    pin->chip = gpiod_chip_open_by_name(chipname);
    if (pin->chip == NULL) {
        fprintf(stderr, "could not open chip %s\n", chipname);
        return -1;
    }

    pin->line = gpiod_chip_get_line(pin->chip, port_nr);
    if (pin->line == NULL) {
        fprintf(stderr, "could not reserve line %s\n", portname);
        return -1;
    }

    if (gpiod_line_request_input(pin->line, consumer)) {
        fprintf(stderr, "could not request input from line %s\n", portname);
        return -1;
    }

    return 0;
}

int run(struct gpio_pin *pin) {
    const int N = 100;
    const int DELAY_USEC = 15;
    struct timespec res, start, curr, log[N];

    CHECK(clock_getres(CLOCK_MONOTONIC, &res));
    CHECK(clock_gettime(CLOCK_MONOTONIC, &start));

    printf("resolution: %ld.%.9ld\n", res.tv_sec, res.tv_nsec);
    printf("start: %ld.%.9ld\n", start.tv_sec, start.tv_nsec);

    curr = start;
    add(&curr, 1000); // Give time for the setup below
    for (int i=0; i<N; i++) {
        log[i] = curr;
        add(&curr, DELAY_USEC);
    }

    struct timespec end;
    int cnt = 0;
    for (int i=0; i<N; i++) {
        CHECK(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &log[i], NULL));
        if (pin->line != NULL) {
            int val = gpiod_line_get_value(pin->line);
            if (val < 0) {
                perror("gpiod_line_get_value");
                return -1;
            }
            cnt += val;
        }
    }
    CHECK(clock_gettime(CLOCK_MONOTONIC, &end));
    printf("Average diff: %ld us\n", diff(&log[0], &end)/(1000*(N-1)));

    return cnt;
}

int main(int argc, char **args) {
    set_rt_sched();
    no_swapping();
    struct gpio_pin pin;
    CHECK(gpio_init(&pin, "gpiochip1", "PC7"));
    
    // run the polling loop
    int res = run(&pin);
    if (res < 0) {
        perror("rtlog");
    } else {
        printf("Res: %d\n", res);
    }

    // Cleanup:
    if (pin.line != NULL) {
        gpiod_line_release(pin.line);
    }
    if (pin.chip != NULL) {
        gpiod_chip_close(pin.chip);
    }
}
