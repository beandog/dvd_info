#include "dvd_vts.h"

/**
 * Functions used to get information about a DVD VTS (a combination of VOBs)
 */

ssize_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, const uint16_t vts_number) {

	ssize_t vts_blocks = 0;
	dvd_file_t *dvdread_vts_file;

	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS);
	vts_blocks = DVDFileSize(dvdread_vts_file);

	return vts_blocks;

}

ssize_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, const uint16_t vts_number) {

	ssize_t vts_blocks = 0;
	ssize_t vts_filesize = 0;

	vts_blocks = dvd_vts_blocks(dvdread_dvd, vts_number);
	vts_filesize = vts_blocks * DVD_VIDEO_LB_LEN;

	return vts_filesize;

}

int dvd_vts_vobs(dvd_reader_t *dvdread_dvd, const uint16_t vts_number) {

	dvd_stat_t dvdread_stat;
	int vts_vobs = 0;

	DVDFileStat(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS, &dvdread_stat);

	vts_vobs = dvdread_stat.nr_parts;

	return vts_vobs;
	
}
