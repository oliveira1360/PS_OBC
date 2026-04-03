#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <stdint.h>

typedef struct
{
    float temperature;
} temperature_data_t;

int temperature_read(temperature_data_t *out);
void temperature_read_async();
void temperature_tick(void);

#endif