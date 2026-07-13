#ifndef DVD_INFO_VTS_H
#define DVD_INFO_VTS_H

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_specs.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vob.h"
#include "dvd_udf.h"

struct dvd_vts {
	uint16_t vts;
	bool valid;
	char id[DVD_VTS_ID + 1];
	uint64_t blocks;
	uint64_t filesize;
	uint64_t filesize_mbs;
	uint16_t vobs;
	uint16_t tracks;
	uint16_t valid_tracks;
	uint16_t invalid_tracks;
	struct dvd_vob dvd_vobs[DVD_MAX_TRACKS];
};

uint64_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

uint64_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

uint64_t dvd_vts_filesize_mbs(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

uint16_t dvd_vts_vobs(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

struct dvd_vts dvd_vts_open(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

#endif
