#include "node.h"

#include <stdio.h>
#include <string.h>

#define max(a, b) (a < b)? b : a
#define min(a, b) (a > b)? b : a

const char *ndl_value_type_to_string[EVAL_SIZE] = {
    "None",   /* EVAL_NONE  */
    "Ref",    /* EVAL_REF   */
    "Sym",    /* EVAL_SYM   */
    "Int",    /* EVAL_INT   */
    "Float"   /* EVAL_FLOAT */
};

#define PRETTYPRINT_BUFFSIZE 128

static void ndl_prettyprint_none(int len, char *buff) {

    int i;
    for (i = 0; i < len; i++)
        buff[i] = ' ';
}

static void ndl_prettyprint_ref(ndl_ref value, int len, char *buff) {

    int i;
    for (i = 0; len - i > 8; i++)
        buff[i] = ' ';

    if (value == NDL_NULL_REF) {
        memcpy(buff + i, "    null", 8);
        return;
    }

    int filteroff = 7;
    uint32_t filter = 0xF0000000;
    for (; !(filter & (unsigned) value) && (i < len - 1); i++) {
        filter >>= 4;
        filteroff--;
        buff[i] = ' ';
    }

    const char ascii[16] = "0123456789ABCDEF";

    for(; i < len; i++) {
        buff[i] = ascii[(filter & (unsigned) value) >> (filteroff*4)];
        filter >>= 4;
        filteroff--;
    }
}

static void ndl_prettyprint_sym(ndl_sym value, int len, char *buff) {

    char *sym = (char*) &value;

    int i;
    for (i = 0; len - i > 8; i++)
        buff[i] = ' ';

    for (; i < len; i++) {
        char t = sym[i - len + 8];
        if (t >= '\0' && t < ' ') t = (char) (t + '0');
        buff[i] = t;
    }
}

static int ndl_prettyprint_getmsd(ndl_float value) {

    if (value < 0) {

        int msd = 0;
        double max = 10;
        while (value > max) {
            max *= 10;
            msd++;
        }

        return msd;

    } else {

        int msd = -1;
        double min = 0.1;
        while (value < min) {
            min *= 0.1;
            msd--;
        }

        return msd;
    }
}

static void ndl_prettyprint_exp_float(ndl_float value, int len, char *buff) {

    /*           d.  ddd e+dd */
    /* int prec = - 2 + len - 4; */

    /* We need a max of two characters to express the precision. */
    /*
    char format[7];
    memcpy(format, "%E2.\0\0", 7);
    printf("%d\n", len);

    if (prec >= 10) {
        format[4] = (prec / 10) + '0';
        format[5] = (prec % 10) + '0';
    } else if (prec > 0) {
        format[4] = prec + '0';
    } else {
        format[3] = '\0';
    }

    char tbuff[PRETTYPRINT_BUFFSIZE];
    snprintf(tbuff, len+1, format, value);
    memcpy(buff, tbuff, len);

    printf("%s: %s@%d\n", format, buff, len);
    */
    int i;
    for (i = 0; i < len; i++)
        buff[i] = '#';
}

static void ndl_prettyprint_float(ndl_float value, int len, char *buff) {

    if (value < 0) {

        if (len >= 1) {
            buff[0] = ' ';
            buff[1] = '-';
        }

        ndl_prettyprint_float(-value, len-2, buff+2);

        return;
    }

    /* Two notations: Normal and exponential.
     * We use exponential if we cannot view both MSB and the "0.00" digits.
     * [ MSD ] ... [ ONES ] . [ DECIMALS ] [ CENTS ] ... [ MIN ]
     */

    /* msdmax <= msd < msdmax. */
    int msdmax = len - 3;    /* Reserve last three digits for ".00". */
    int msdmin = -(len - 2); /* Reserve the first two digits for "0.". */

    int msd = ndl_prettyprint_getmsd(value);

    if (msd >= msdmax || msd < msdmin) {
        ndl_prettyprint_exp_float(value, len, buff);
        return;
    }

    /* 00..000.000...00, starting from the MSB. */
    char size = (char) (max(msd + 1, 1));
    char prec = (char) (len - size - 1);

    char format[8];
    memcpy(format, "%f\0\0\0\0\0", 8);

    int taken = 0;
    if (size >= 10) {
        format[2] = (char) (size / 10 + '0');
        format[3] = (char) (size % 10 + '0');
        taken = 2;
    } else {
        format[2] = (char) (size + '0');
        taken = 1;
    }

    format[2+taken] = '.';

    if (prec >= 10) {
        format[3+taken] = (char) (prec / 10 + '0');
        format[4+taken] = (char) (prec % 10 + '0');
    } else {
        format[3+taken] = (char) (prec + '0');
    }

    char tbuff[PRETTYPRINT_BUFFSIZE];
    snprintf(tbuff, (unsigned) len+1, format, value);
    memcpy(buff, tbuff, (unsigned) len);
}

static void ndl_prettyprint_int(ndl_int value, int len, char *buff) {

    if (value < 0) {

        if (len >= 2) {
            buff[0] = ' ';
            buff[1] = '-';
        }

        ndl_prettyprint_int(-value, len-2, buff+2);

        return;
    }

    char tbuff[PRETTYPRINT_BUFFSIZE];
    snprintf(tbuff, PRETTYPRINT_BUFFSIZE, "%d", (int) value);
    int slen = (signed) strlen(tbuff);

    if (slen > len) {
        ndl_prettyprint_exp_float((ndl_float) value, len, buff);
        return;
    }

    int i;
    for (i = 0; i < (len - slen); i++)
        buff[i] = ' ';

    for (; i < len; i++)
        buff[i] = tbuff[i - len + slen];
}

int ndl_value_to_string(ndl_value value, int len, char *buff) {

    /* Always return the same length. Example strings:
     * "[Ref:    AB73D]" (Makes leading zeros spaces.)
     * "[Sym: hello   ]"
     * "[Int:  8675305]"
     * "[Int:-3.4255e2]" (If too large to display, show abbreviated exp notation.)
     * "[Float:-3.1415]"
     * "[Float:-3.1e10]"
     * "[None:        ]"
     * "[Ref:     NULL]"
     */

    const char *type = ndl_value_type_to_string[value.type];
    int typelen = (signed) strlen(type);

    buff[0] = '[';
    memcpy(buff + 1, type, (unsigned) (min(typelen, len - 1)));
    buff[min(1 + typelen, len - 1)] = ':';
    buff[len - 1] = ']';

    int nlen = len - typelen - 3;
    char *nbuff = buff + typelen + 2;

    switch (value.type) {

    case EVAL_NONE:
        ndl_prettyprint_none(nlen, nbuff);
        break;
    case EVAL_REF:
        ndl_prettyprint_ref(value.ref, nlen, nbuff);
        break;
    case EVAL_SYM:
        ndl_prettyprint_sym(value.sym, nlen, nbuff);
        break;
    case EVAL_INT:
        ndl_prettyprint_int(value.num, nlen, nbuff);
        break;
    case EVAL_FLOAT:
        ndl_prettyprint_float(value.real, nlen, nbuff);
        break;
    default:
        ndl_prettyprint_none(nlen, nbuff);
        break;
    }

    return len;
}
