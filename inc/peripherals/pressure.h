#ifndef PRESSURE_H
#define PRESSURE_H

#include <stdint.h>

typedef struct {
    float pressure;
} pressure_data_t; /* 4 bytes */

int  pressure_read(pressure_data_t *out);
void pressure_read_async(void);
void pressure_tick(void);

#endif /* PRESSURE_H */
