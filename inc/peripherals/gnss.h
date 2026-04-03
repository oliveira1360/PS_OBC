#ifndef GNSS_H
#define GNSS_H

#include <stdint.h>

typedef struct
{
    float latitude;  /* graus decimais */
    float longitude; /* graus decimais */
    float altitude;  /* metros                      */
    float speed;     /* m/s                         */
} gnss_data_t;

int gnss_read(gnss_data_t *out);
void gnss_read_async();
void gnss_tick(void);

#endif