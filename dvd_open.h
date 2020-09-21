#ifndef DVD_INFO_DVD_OPEN_H
#define DVD_INFO_DVD_OPEN_H

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_device.h"
#include "dvd_drive.h"
#include "dvd_vmg_ifo.h"
#include "dvd_info.h"

int device_open(const char *device_filename);

struct dvd_info dvd_info_open(dvd_reader_t *dvdread_dvd, const char *device_filename);

#endif
