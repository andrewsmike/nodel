#include "test.h"

#include "ndltime.h"

#include <limits.h>

char *ndl_test_time_conv(void) {

    int64_t times[7] = {
        LLONG_MIN,
        -1000000,
        -1,
        0,
        1,
        10000000,
        LLONG_MAX
    };

    int i;
    for (i = 0; i < 7; i++) {

        ndl_time t = ndl_time_from_usec(times[i]);
        if (ndl_time_to_usec(t) != times[i])
            return "Time conversion changes values";
    }

    return NULL;
}

char *ndl_test_time_add(void) {

    ndl_time t = ndl_time_from_usec(1000);
    ndl_time tp = ndl_time_add(t, NDL_TIME_ZERO);
    if (ndl_time_cmp(t, tp) != 0)
        return "Adding zero changes time";

    ndl_time delta = ndl_time_from_usec(-1);
    tp = ndl_time_add(t, delta);
    if (ndl_time_cmp(t, tp) <= 0)
        return "Adding negative time does not enter past";

    tp = ndl_time_sub(t, delta);
    if (ndl_time_cmp(t, tp) >= 0)
        return "Subtracting negative time does not enter future";

    tp = ndl_time_add(tp, delta);
    if (ndl_time_cmp(t, tp) != 0)
        return "Adding and subtracting delta changes value";

    return NULL;
}

char *ndl_test_time_get(void) {

    ndl_time start = ndl_time_get();

    int sum = 0;
    int i;
    for (i = 0; i < 10000; i++)
        sum = (sum + i) % 3;

    ndl_time end = ndl_time_get();

    ndl_time interval = ndl_time_sub(end, start);
    if (ndl_time_cmp(interval, NDL_TIME_ZERO) < 0)
        return "Gave nonsequential values for the time";

    return NULL;
}
