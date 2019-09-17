#ifndef DVD_INFO_TIME_H
#define DVD_INFO_TIME_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dvdread/ifo_types.h"

uint32_t dvd_time_to_milliseconds(dvd_time_t *dvd_time);

void milliseconds_length_format(char *dest_str, const uint32_t milliseconds);

#endif
