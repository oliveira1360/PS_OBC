#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <stdint.h>

typedef struct
{
    float temperature;
} temperature_data_t; /* 4 bytes */

int  temperature_read(temperature_data_t *out);
void temperature_read_async(void);
void temperature_tick(void);

#endif /* TEMPERATURE_H */
