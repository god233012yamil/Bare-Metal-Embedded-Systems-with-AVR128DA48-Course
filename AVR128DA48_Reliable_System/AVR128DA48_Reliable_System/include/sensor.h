#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "status.h"

typedef struct
{
    int16_t temperature_c_x10;
    bool valid;
} sensor_data_t;

void sensor_init(void);
void sensor_task(void);
status_t sensor_read(sensor_data_t *data);
const sensor_data_t *sensor_get_latest(void);

#endif
