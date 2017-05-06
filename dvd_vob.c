#include "dvd_vob.h"

/**
 * Functions used to get information about a DVD VOB
 */

ssize_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number) {

	if(!vob_number)
		return 0;

	ssize_t vob_blocks = 0;
	ssize_t vob_filesize = 0;

	vob_filesize = dvd_vob_filesize(dvdread_dvd, vts_number, vob_number);
	vob_blocks = vob_filesize / DVD_VIDEO_LB_LEN;

	return vob_blocks;

}

ssize_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number, const uint16_t vob_number) {

	if(!vob_number)
		return 0;

	dvd_stat_t dvdread_stat;
	ssize_t vob_filesize = 0;

	DVDFileStat(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS, &dvdread_stat);
	vob_filesize = dvdread_stat.parts_size[vob_number - 1];

	return vob_filesize;

}
