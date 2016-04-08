#ifndef NODEL_ENDIAN_H
#define NODEL_ENDIAN_H

/* Numbers must be unsigned. */
#define ENDIAN_SWAP_16(number)                  \
    ((((number) & 0xFF00) >> 8)                 \
     | ((number) & 0x00FF) << 8)

#define ENDIAN_SWAP_32(number)                          \
    (ENDIAN_SWAP_16(((number) & 0xFFFF0000) >> 16)      \
     | ENDIAN_SWAP_16((number) & 0x0000FFFF) << 16)

#define ENDIAN_SWAP_64(number)                                  \
    (ENDIAN_SWAP_32(((number) & 0xFFFFFFFF00000000) >> 32)      \
     | ENDIAN_SWAP_32((number) & 0x00000000FFFFFFFF) << 32)


#ifdef LITTLE_ENDIAN

#    define ENDIAN_TO_BIG_16(number) number
#    define ENDIAN_TO_BIG_32(number) number
#    define ENDIAN_TO_BIG_64(number) number

#    define ENDIAN_FROM_BIG_16(number) number
#    define ENDIAN_FROM_BIG_32(number) number
#    define ENDIAN_FROM_BIG_64(number) number

#else

#    define ENDIAN_TO_BIG_16(number) ENDIAN_SWAP_16(number)
#    define ENDIAN_TO_BIG_32(number) ENDIAN_SWAP_32(number)
#    define ENDIAN_TO_BIG_64(number) ENDIAN_SWAP_64(number)

#    define ENDIAN_FROM_BIG_16(number) ENDIAN_SWAP_16(number)
#    define ENDIAN_FROM_BIG_32(number) ENDIAN_SWAP_32(number)
#    define ENDIAN_FROM_BIG_64(number) ENDIAN_SWAP_64(number)

#endif

#endif /* NODEL_ENDIAN_H */
