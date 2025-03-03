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

double dvd_vts_filesize_mbs(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	ssize_t vts_blocks = 0;
	vts_blocks = dvd_vts_blocks(dvdread_dvd, vts_number);

	if(vts_blocks == 0)
		return 0;

	double vts_filesize_mbs = 0;
	vts_filesize_mbs = ceil((vts_blocks * DVD_VIDEO_LB_LEN) / 1048576.0);

	return vts_filesize_mbs;

}

/**
 * With libdvdread 6.1.3 on MSYS2, DVDFileStat() will see the VOB files, but
 * the dvd_stat_t struct nr_parts value returns 0. Using the number inside of
 * vts_ifo directly. However, on Linux and friends, this does *not* work for
 * for some reason. Just accessing the variable causes strange problems, and
 * the filesize can be completely incorrect. In addition, nr_of_vobs is one
 * too many on Linux for some reason.
 *
 * I've spent a *lot* of time debugging it, but haven't been able to find any
 * reasons why there is unusual behavior. Suffice it to say, this works. This
 * is also how the original program 'dvdbackup' gets the number of vobs.
 *
 * Using DVDFileStat is safer anyway for dvd_backup, to make sure that the
 * files are actually there and can be accessed.
 */
uint16_t dvd_vts_vobs(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	uint16_t vts_vobs = 0;

#if defined (__MINGW32__) || defined (__CYGWIN__) || defined (__MSYS__)

	ifo_handle_t *vts_ifo = NULL;
	vts_ifo = ifoOpen(dvdread_dvd, vts_number);

	if(vts_ifo == NULL)
		return 0;

	if(vts_ifo->vts_c_adt == NULL)
		return 0;

	vts_vobs = vts_ifo->vts_c_adt->nr_of_vobs;

	return vts_vobs;

#else

	dvd_stat_t dvdread_stat;

	int retval = 0;
	retval = DVDFileStat(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS, &dvdread_stat);
	if(retval < 0)
		return 0;

	vts_vobs = dvdread_stat.nr_parts;

	return vts_vobs;

#endif

}

struct dvd_vts dvd_vts_open(dvd_reader_t *dvdread_dvd, uint16_t vts) {

	struct dvd_vts dvd_vts;

	dvd_vts.vts = vts;

	// Initialize to defaults
	dvd_vts.valid = false;
	dvd_vts.blocks = 0;
	dvd_vts.filesize = 0;
	dvd_vts.filesize_mbs = 0;
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
	dvd_vts.filesize_mbs = dvd_vts_filesize_mbs(dvdread_dvd, vts);

	dvd_vts.valid = true;

	return dvd_vts;

}
