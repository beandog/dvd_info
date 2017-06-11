#ifndef DVD_INFO_VOB_H
#define DVD_INFO_VOB_H

#include "dvd_info.h"

struct dvd_vob {
	uint16_t vts;
	uint16_t vob;
	ssize_t blocks;
	ssize_t filesize;
};

ssize_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

ssize_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

#endif
