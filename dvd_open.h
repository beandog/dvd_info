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

void dvd_info_logger_cb(void *p, dvd_logger_level_t dvdread_log_level, const char *msg, va_list dvd_log_va);

void dvd_info_logger_cb_debug(void *p, dvd_logger_level_t dvdread_log_level, const char *msg, va_list dvd_log_va);

int device_open(const char *device_filename);

struct dvd_info dvd_info_open(dvd_reader_t *dvdread_dvd, const char *device_filename);

#endif
