#ifndef _DVD_VTS_H_
#define _DVD_VTS_H_

#include "dvd_info.h"

ssize_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

ssize_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

int dvd_vts_vobs(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

#endif
