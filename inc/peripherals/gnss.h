#ifndef GNSS_H
#define GNSS_H

#include <stdint.h>

typedef struct
{
    float latitude;  /* graus decimais */
    float longitude; /* graus decimais */
    float altitude;  /* metros         */
    float speed;     /* m/s            */
} gnss_data_t;       /* 128 bits / 16 bytes */

int  gnss_read(gnss_data_t *out);
void gnss_read_async(void);
void gnss_tick(void);

#endif /* GNSS_H */
