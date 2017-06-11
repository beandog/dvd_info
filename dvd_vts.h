#ifndef DVD_INFO_VTS_H
#define DVD_INFO_VTS_H

#include <stdint.h>
#include <dvdread/dvd_reader.h>
#include "dvd_specs.h"

struct dvd_vts {
	uint16_t vts;
	char id[DVD_VTS_ID + 1];
	ssize_t blocks;
	ssize_t filesize;
	int vobs;
};

ssize_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

ssize_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

int dvd_vts_vobs(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

#endif
