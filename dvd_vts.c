#include "dvd_vts.h"

/**
 * Functions used to get information about a DVD VTS (a combination of VOBs)
 */

ssize_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	dvd_file_t *dvdread_vts_file;
	dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS);

	if(dvdread_vts_file == 0)
		return 0;

	ssize_t vts_blocks = 0;
	vts_blocks = DVDFileSize(dvdread_vts_file);

	if(vts_blocks < 0)
		return 0;

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

struct dvd_vts dvd_vts_open(dvd_reader_t *dvdread_dvd, uint16_t vts) {

	struct dvd_vts dvd_vts;

	dvd_vts.vts = vts;

	// Initialize to defaults
	dvd_vts.valid = false;
	dvd_vts.blocks = 0;
	dvd_vts.filesize = 0;
	dvd_vts.vobs = 0;
	dvd_vts.tracks = 0;
	dvd_vts.valid_tracks = 0;
	dvd_vts.invalid_tracks = 0;

	// First VTS is the VMG IFO, used here only as a placeholder
	if(vts == 0)
		return dvd_vts;

	ifo_handle_t *vts_ifo = NULL;
	vts_ifo = ifoOpen(dvdread_dvd, vts);

	if(vts_ifo == NULL)
		return dvd_vts;

	// TODO needs more testing, and also move into a function that examines if VTS is valid or not
	/*
	if(vts_ifos[vts]->vtsi_mat->vts_tmapt == 0) {
		dvd_vts[vts].valid = false;
		continue;
	}
	*/

	if(!ifo_is_vts(vts_ifo))
		return dvd_vts;

	dvd_vts.blocks = dvd_vts_blocks(dvdread_dvd, vts);

	if(!dvd_vts.blocks)
		return dvd_vts;

	dvd_vts.vobs = dvd_vts_vobs(dvdread_dvd, vts);
	dvd_vts.filesize = dvd_vts_filesize(dvdread_dvd, vts);

	dvd_vts.valid = true;

	return dvd_vts;

}
