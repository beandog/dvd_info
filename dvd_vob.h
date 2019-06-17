#ifndef DVD_INFO_VOB_H
#define DVD_INFO_VOB_H

#include <stdint.h>
#include <dvdread/dvd_reader.h>

struct dvd_vob {
	uint16_t vts;
	uint16_t vob;
	uint64_t blocks;
	uint64_t filesize;
};

uint64_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

uint64_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number);

#endif
