#ifndef DVD_INFO_DRIVE_H
#define DVD_INFO_DRIVE_H

#include <stdio.h>
#include "dvd_device.h"

int dvd_drive_get_status(const char *device_filename);

bool dvd_drive_has_media(const char *device_filename);

bool dvd_drive_is_open(const char *device_filename);

bool dvd_drive_is_closed(const char *device_filename);

bool dvd_drive_is_ready(const char *device_filename);

void dvd_drive_display_status(const char *device_filename);

#endif
