#include "ndltime.h"

#include <stdio.h>

#define GIGA 1000000000
#define MEGA 1000000
#define KILO 1000

ndl_time ndl_time_add(ndl_time a, ndl_time b) {

    ndl_time ret;
    ret.tv_sec = a.tv_sec + b.tv_sec;
    ret.tv_nsec = a.tv_nsec + b.tv_nsec;

    if (ret.tv_nsec > GIGA) {
        ret.tv_nsec -= GIGA;
        ret.tv_sec++;
    }

    return ret;
}

ndl_time ndl_time_sub(ndl_time a, ndl_time b) {

    ndl_time ret;
    ret.tv_sec = a.tv_sec - b.tv_sec;
    ret.tv_nsec = a.tv_nsec - b.tv_nsec;

    if (ret.tv_nsec < 0) {
        ret.tv_nsec += GIGA;
        ret.tv_sec--;
    }

    return ret;
}

int ndl_time_cmp(ndl_time a, ndl_time b) {

    if (a.tv_sec < b.tv_sec)
        return -1;
    else if (a.tv_sec > b.tv_sec)
        return 1;
    else if (a.tv_nsec < b.tv_nsec)
        return -1;
    else if (a.tv_nsec > b.tv_nsec)
        return 1;

    return 0;
}

int64_t ndl_time_to_usec(ndl_time duration) {

    int64_t ret = 0;
    ret = duration.tv_sec * MEGA;
    ret += duration.tv_nsec / KILO;

    return ret;
}

ndl_time ndl_time_from_usec(int64_t duration) {

    struct timespec ret;
    ret.tv_sec = duration / MEGA;
    ret.tv_nsec = (duration % MEGA) * KILO;

    if (ret.tv_nsec < 0) {
        ret.tv_nsec += GIGA;
        ret.tv_sec--;
    }

    return ret;
}

ndl_time ndl_time_get(void) {

    ndl_time ret;
    int err = clock_gettime(CLOCK_MONOTONIC, &ret);
    if (err != 0)
        return NDL_TIME_ZERO;

    return ret;
}

void ndl_time_print(ndl_time time) {

    printf("Printing time:\n");
    printf("%lds%ldns\n", time.tv_sec, time.tv_nsec);
}
