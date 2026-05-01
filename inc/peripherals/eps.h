#ifndef EPS_H
#define EPS_H

#include <stdint.h>

typedef struct
{
    float voltage;
    float current;
} eps_data_t; /* 64 bits / 8 bytes */

void eps_read_async(void);
void eps_tick(void);

#endif /* EPS_H */
