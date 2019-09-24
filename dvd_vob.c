#include "dvd_vob.h"

/**
 * Functions used to get information about a DVD VOB
 */

uint64_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number) {

	if(!vob_number)
		return 0;

	uint64_t vob_blocks = 0;
	uint64_t vob_filesize = 0;

	vob_filesize = dvd_vob_filesize(dvdread_dvd, vts_number, vob_number);
	vob_blocks = vob_filesize / DVD_VIDEO_LB_LEN;

	return vob_blocks;

}

uint64_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number) {

	if(!vob_number)
		return 0;

	dvd_stat_t dvdread_stat;

	int retval;
	retval = DVDFileStat(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS, &dvdread_stat);

	if(retval < 0)
		return 0;

	uint64_t vob_filesize = 0;
	vob_filesize = (uint64_t)dvdread_stat.parts_size[vob_number - 1];

	return vob_filesize;

}
