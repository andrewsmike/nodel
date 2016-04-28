#ifndef NODEL_TIME_H
#define NODEL_TIME_H

#include <stdint.h>
#include <time.h>

/* Convenience functions for time manipulation.
 * Mostly used by clock event system, runtime.
 */
typedef struct timespec ndl_time;

#define NDL_TIME_ZERO ((ndl_time) {.tv_sec = 0, .tv_nsec = 0})

/* Basic time manipulation.
 * Resulting .tv_nsec is always positive.
 *
 * add() adds two times.
 * sub() subtracts two times.
 *
 * cmp() returns -1:a<b, 0:a==b, 1:a>b.
 */
ndl_time ndl_time_add(ndl_time a, ndl_time b);
ndl_time ndl_time_sub(ndl_time a, ndl_time b);

int      ndl_time_cmp(ndl_time a, ndl_time b);

/* Conversion to/from microseconds.
 * Uses 64 bits, so wraparound date is
 * a couple hundred thousand years from now.
 * Resulting .tv_nsec is always positive.
 *
 * to_usec() takes an ndl_time and converts it to microseconds.
 * from_usec() takes microseconds and converts it to an ndl_time.
 */
int64_t  ndl_time_to_usec(ndl_time duration);
ndl_time ndl_time_from_usec(int64_t duration);

/* Get current time from the system.
 * Uses CLOCK_MONOTONIC.
 * Returns NDL_TIME_ZERO on error.
 */
ndl_time ndl_time_get(void);

/* Print a time. */
void ndl_time_print(ndl_time time);


#endif /* NODEL_TIME_H */
