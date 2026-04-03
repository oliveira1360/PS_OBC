#ifndef PRESSURE_H
#define PRESSURE_H

#include <stdint.h>

typedef struct {
    float pressure;
} pressure_data_t;

int  pressure_read(pressure_data_t *out);
void pressure_read_async();
void pressure_tick(void);

#endif