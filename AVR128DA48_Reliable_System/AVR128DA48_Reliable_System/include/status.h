#ifndef STATUS_H_
#define STATUS_H_

#include <stdint.h>

typedef enum
{
    STATUS_OK = 0,
    STATUS_ERROR,
    STATUS_TIMEOUT,
    STATUS_INVALID_ARG,
    STATUS_BUSY,
    STATUS_HW_ERROR,
    STATUS_CRC_ERROR
} status_t;

#endif
