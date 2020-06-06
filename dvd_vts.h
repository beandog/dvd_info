#ifndef DVD_INFO_VTS_H
#define DVD_INFO_VTS_H

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include "dvd_specs.h"
#include "dvd_vmg_ifo.h"
#include "dvd_vob.h"

struct dvd_vts {
	uint16_t vts;
	bool valid;
	char id[DVD_VTS_ID + 1];
	ssize_t blocks;
	ssize_t filesize;
	int vobs;
	uint16_t tracks;
	uint16_t valid_tracks;
	uint16_t invalid_tracks;
	struct dvd_vob dvd_vobs[99];
};

ssize_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

ssize_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

int dvd_vts_vobs(dvd_reader_t *dvdread_dvd, uint16_t vts_number);

struct dvd_vts dvd_vts_open(dvd_reader_t *dvdread_dvd, uint16_t vts);

#endif
