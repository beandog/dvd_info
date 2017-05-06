#ifndef _DVD_VOB_H_
#define _DVD_VOB_H_

#include "dvd_info.h"

ssize_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

ssize_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

#endif
