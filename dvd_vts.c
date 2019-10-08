#include "dvd_vts.h"

/**
 * Functions used to get information about a DVD VTS (a combination of VOBs)
 */

ssize_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	dvd_file_t *dvdread_vts_file;
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS);

	if(dvdread_vts_file == 0)
		return 0;

	ssize_t vts_filesize = 0;
	vts_filesize = DVDFileSize(dvdread_vts_file);

	if(vts_filesize < 0)
		return 0;

	ssize_t vts_blocks = 0;
	vts_blocks = vts_filesize / DVD_VIDEO_LB_LEN;

	return vts_blocks;

}

ssize_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	ssize_t vts_blocks = 0;
	vts_blocks = dvd_vts_blocks(dvdread_dvd, vts_number);

	ssize_t vts_filesize = 0;
	vts_filesize = vts_blocks * DVD_VIDEO_LB_LEN;

	return vts_filesize;

}

int dvd_vts_vobs(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	dvd_stat_t dvdread_stat;

	int retval = 0;
	retval = DVDFileStat(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS, &dvdread_stat);
	if(retval < 0)
		return 0;

	int vts_vobs = 0;
	vts_vobs = dvdread_stat.nr_parts;

	return vts_vobs;

}
