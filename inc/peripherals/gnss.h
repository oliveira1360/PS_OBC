#ifndef GNSS_H
#define GNSS_H

typedef struct
{
    float    latitude;       // graus decimais, ex: 38.7169
    float    longitude;      // graus decimais, ex: -9.1390
    float    altitude;       // metros
    float    speed;          // m/s
 
} gnss_data_t;

int gnss_read(gnss_data_t *out);

#endif