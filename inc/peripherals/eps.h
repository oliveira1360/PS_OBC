#ifndef EPS_H
#define EPS_H

#include <stdint.h>

typedef struct {
    float voltage;
    float current;
} eps_data_t;

void eps_read_async();
void eps_tick(void);

#endif