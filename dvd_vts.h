#ifndef DVD_INFO_VTS_H
#define DVD_INFO_VTS_H

#include <stdint.h>
#include <stdbool.h>
#include <dvdread/dvd_reader.h>
#include "dvd_specs.h"

struct dvd_vts {
	uint16_t vts;
	bool valid;
	char id[DVD_VTS_ID + 1];
	uint64_t blocks;
	uint64_t filesize;
	int vobs;
	uint16_t tracks;
	uint16_t valid_tracks;
	uint16_t invalid_tracks;
};

uint64_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

uint64_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

int dvd_vts_vobs(dvd_reader_t *dvdread_dvd, const uint16_t vts_number);

#endif
