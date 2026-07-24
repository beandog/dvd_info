#ifndef DVD_INFO_VOB_H
#define DVD_INFO_VOB_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <dvdread/dvd_reader.h>
#include "dvd_udf.h"

struct dvd_vob {
	uint16_t vts;
	uint16_t vob;
	uint64_t blocks;
	uint64_t filesize;
	uint64_t filesize_mbs;
	char udf_filename[PATH_MAX];
};

/**
 * Gets the block size of a VOB in a VTS
 *
 * A simple helper function around dvd_vob_filesize to do the math of number of
 * blocks, which is the filesize divided by the block size (2 KiB), or DVD_VIDEO_LB_LEN
 *
 */
uint64_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number);

/**
 * Checks a VOB in a VTS to get its filesize.
 *
 * If the VOB # is zero, then it looks at the menu VOB: VIDEO_TS.VOB or VTS_XX_0.VOB
 * Otherwise, it looks at the title VOB: VTS_XX_[1-9].VOB
 *
 * A menu vob may or may not be present on a disc, so this can safely be run to check
 * if it's zero or not.
 *
 */
uint64_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number);

/**
 * Helper function to get filesize of a VOB in MBs.
 */
uint64_t dvd_vob_filesize_mbs(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number);

/**
 * Create and populate a dvd_vob struct
 */
struct dvd_vob dvd_vob_open(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number);

#endif
