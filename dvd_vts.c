#include "dvd_vts.h"

/**
 * Functions used to get information about a DVD Video Title Set
 */

uint64_t dvd_vts_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	bool udf = false;

	if(vts_number == 0)
		udf = true;

#if defined (__MINGW32__) || defined (__CYGWIN__) || defined (__MSYS__)
	udf = true;
#endif

	uint64_t vts_blocks = 0;

	if(udf) {

		uint16_t vob_number = 0;
		char udf_filename[PATH_MAX];
		struct dvd_udf_file_t dvd_udf_file;

		uint16_t max_vobs = 9;
		if(vts_number == 0)
			max_vobs = 1;

		for(vob_number = 1; vob_number <= max_vobs; vob_number++) {

			memset(udf_filename, '\0', PATH_MAX);

			if(vts_number == 0)
				strncpy(udf_filename, "/VIDEO_TS/VIDEO_TS.VOB", PATH_MAX);
			else
				snprintf(udf_filename, PATH_MAX, "/VIDEO_TS/VTS_%02" PRIu16 "_%" PRIu16 ".VOB", vts_number, vob_number);

			dvd_udf_file = dvd_udf_file_open(dvdread_dvd, udf_filename);

			if(dvd_udf_file.blocks)
				vts_blocks += dvd_udf_file.blocks;
			else
				break;

		}

	}

	if(!udf) {

		dvd_file_t *dvdread_vts_file;
		dvdread_vts_file = DVDOpenFile(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS);

		if(dvdread_vts_file == 0)
			return 0;

		ssize_t dvdread_file_blocks = 0;
		// Despite the name of the function, it returns number of blocks, not bytes
		dvdread_file_blocks = DVDFileSize(dvdread_vts_file);

		if(dvdread_file_blocks < 0)
			return 0;

		vts_blocks = (uint64_t)dvdread_file_blocks;

	}

	return vts_blocks;

}

uint64_t dvd_vts_filesize(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	uint64_t vts_blocks = 0;
	vts_blocks = dvd_vts_blocks(dvdread_dvd, vts_number);

	uint64_t vts_filesize = 0;
	vts_filesize = vts_blocks * DVD_VIDEO_LB_LEN;

	return vts_filesize;

}

uint64_t dvd_vts_filesize_mbs(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	uint64_t vts_blocks = 0;
	vts_blocks = dvd_vts_blocks(dvdread_dvd, vts_number);

	if(vts_blocks == 0)
		return 0;

	uint64_t vts_filesize_mbs = 0;
	vts_filesize_mbs = (vts_blocks * DVD_VIDEO_LB_LEN);

	return vts_filesize_mbs;

}

/**
 * With libdvdread 7.0.1 on MSYS2, DVDFileStat() will see the VOB files, but
 * the dvd_stat_t struct nr_parts value returns 0. In that case, just look at
 * all the possible UDF filenames and get those instead.
 */
uint16_t dvd_vts_vobs(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	if(vts_number == 0)
		return 1;

	uint16_t vts_vobs = 0;

	bool udf = false;

#if defined (__MINGW32__) || defined (__CYGWIN__) || defined (__MSYS__)
	udf = true;
#endif

	if(udf) {

		uint8_t vob_number = 0;
		char udf_filename[PATH_MAX];
		struct dvd_udf_file_t dvd_udf_file;

		for(vob_number = 1; vob_number <= 9; vob_number++) {

			if(vts_number == 0)
				strncpy(udf_filename, "/VIDEO_TS/VIDEO_TS.VOB", PATH_MAX);
			else
				snprintf(udf_filename, PATH_MAX, "/VIDEO_TS/VTS_%02" PRIu16 "_%" PRIu16 ".VOB", vts_number, vob_number);

			dvd_udf_file = dvd_udf_file_open(dvdread_dvd, udf_filename);

			if(dvd_udf_file.filesize)
				vts_vobs++;
			else
				break;

		}

	}

	if(!udf) {

		dvd_stat_t dvdread_stat;

		int retval = 0;
		retval = DVDFileStat(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS, &dvdread_stat);
		if(retval < 0)
			return 0;

		if(dvdread_stat.nr_parts > 0)
			vts_vobs = (uint16_t)dvdread_stat.nr_parts;

	}

	return vts_vobs;

}

struct dvd_vts dvd_vts_open(dvd_reader_t *dvdread_dvd, uint16_t vts_number) {

	struct dvd_vts dvd_vts;

	dvd_vts.vts = vts_number;

	// Initialize to defaults
	dvd_vts.blocks = 0;
	dvd_vts.filesize = 0;
	dvd_vts.filesize_mbs = 0;
	dvd_vts.vobs = 0;
	dvd_vts.tracks = 0;
	dvd_vts.valid_tracks = 0;
	dvd_vts.invalid_tracks = 0;

	// I haven't found a way a VTS can be invalid yet
	dvd_vts.valid = true;

	// First VTS is the VMG IFO, used here only as a placeholder
	// if(vts_number == 0)
	//	return dvd_vts;

	ifo_handle_t *vts_ifo = NULL;
	vts_ifo = ifoOpen(dvdread_dvd, vts_number);

	if(vts_ifo == NULL)
		return dvd_vts;

	// I said this needed more testing, but I've never run into it being a problem.
	/*
	if(vts_ifos[vts]->vtsi_mat->vts_tmapt == 0) {
		dvd_vts[vts].valid = false;
		continue;
	}
	*/

	// if(!ifo_is_vts(vts_ifo))
	//	return dvd_vts;

	dvd_vts.blocks = dvd_vts_blocks(dvdread_dvd, vts_number);

	if(!dvd_vts.blocks)
		return dvd_vts;

	double mbs = 0;

	if(vts_number == 0)
		dvd_vts.vobs = 1;
	else
		dvd_vts.vobs = dvd_vts_vobs(dvdread_dvd, vts_number);

	dvd_vts.filesize = dvd_vts.blocks * DVD_VIDEO_LB_LEN;

	mbs = ceil((dvd_vts.blocks * DVD_VIDEO_LB_LEN) / 1048576.0);
	dvd_vts.filesize_mbs = (uint64_t)mbs;

	return dvd_vts;

}
