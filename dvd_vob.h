#ifndef DVD_INFO_VOB_H
#define DVD_INFO_VOB_H

#include "dvd_info.h"

ssize_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

ssize_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

#endif
