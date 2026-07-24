#include "dvd_vob.h"

/**
 * Functions used to get information about a DVD VOB
 */

uint64_t dvd_vob_blocks(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number) {

	uint64_t vob_blocks = 0;

	bool udf = true;

#if defined (__MINGW32__) || defined (__CYGWIN__) || defined (__MSYS__)
	udf = true;
#endif

	if(vts_number == 0)
		udf = true;

	if(udf) {

		char udf_filename[PATH_MAX];
		struct dvd_udf_file_t dvd_udf_file;

		memset(udf_filename, '\0', PATH_MAX);

		if(vts_number == 0)
			strncpy(udf_filename, "/VIDEO_TS/VIDEO_TS.VOB", PATH_MAX);
		else
			snprintf(udf_filename, PATH_MAX, "/VIDEO_TS/VTS_%02" PRIu16 "_%" PRIu16 ".VOB", vts_number, vob_number);

		dvd_udf_file = dvd_udf_file_open(dvdread_dvd, udf_filename);

		vob_blocks = dvd_udf_file.blocks;

	}

	if(!udf) {

		uint64_t vob_filesize = 0;

		vob_filesize = dvd_vob_filesize(dvdread_dvd, vts_number, vob_number);
		if(vob_filesize > 0)
			vob_blocks = vob_filesize / DVD_VIDEO_LB_LEN;

	}

	return vob_blocks;

}

uint64_t dvd_vob_filesize(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number) {

	uint64_t vob_filesize = 0;

	bool udf = true;

#if defined (__MINGW32__) || defined (__CYGWIN__) || defined (__MSYS__)
	udf = true;
#endif

	if(vts_number == 0)
		udf = true;

	if(udf) {

		char udf_filename[PATH_MAX];
		struct dvd_udf_file_t dvd_udf_file;

		memset(udf_filename, '\0', PATH_MAX);

		if(vts_number == 0)
			strncpy(udf_filename, "/VIDEO_TS/VIDEO_TS.VOB", PATH_MAX);
		else
			snprintf(udf_filename, PATH_MAX, "/VIDEO_TS/VTS_%02" PRIu16 "_%" PRIu16 ".VOB", vts_number, vob_number);

		dvd_udf_file = dvd_udf_file_open(dvdread_dvd, udf_filename);

		vob_filesize = dvd_udf_file.filesize;

	}

	if(!udf) {

		dvd_stat_t dvdread_stat;
		int retval = -1;

		off_t dvdread_parts_size = 0;

		if(vob_number == 0) {
			retval = DVDFileStat(dvdread_dvd, vts_number, DVD_READ_MENU_VOBS, &dvdread_stat);
			if(retval == 0)
				dvdread_parts_size = dvdread_stat.parts_size[0];
		} else {
			retval = DVDFileStat(dvdread_dvd, vts_number, DVD_READ_TITLE_VOBS, &dvdread_stat);
			if(retval == 0)
				dvdread_parts_size = dvdread_stat.parts_size[vob_number - 1];
		}

		if(dvdread_parts_size > 0)
			vob_filesize = (uint64_t)dvdread_parts_size;

	}

	return vob_filesize;

}

uint64_t dvd_vob_filesize_mbs(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number) {

	uint64_t blocks;
	blocks = dvd_vob_blocks(dvdread_dvd, vts_number, vob_number);

	if(blocks == 0)
		return 0;

	double mbs = 0;
	mbs = ceil((blocks * DVD_VIDEO_LB_LEN) / 1048576.0);

	uint64_t vob_filesize_mbs = 0;
	vob_filesize_mbs = (uint64_t)mbs;

	return vob_filesize_mbs;

}

struct dvd_vob dvd_vob_open(dvd_reader_t *dvdread_dvd, uint16_t vts_number, uint16_t vob_number) {

	struct dvd_vob dvd_vob;
	double mbs = 0;

	dvd_vob.vts = vts_number;
	dvd_vob.vob = vob_number;
	dvd_vob.blocks = dvd_vob_blocks(dvdread_dvd, vts_number, vob_number);
	dvd_vob.filesize = dvd_vob.blocks * DVD_VIDEO_LB_LEN;

	if(vts_number == 0)
		strncpy(dvd_vob.udf_filename, "/VIDEO_TS/VIDEO_TS.VOB", PATH_MAX);
	else
		snprintf(dvd_vob.udf_filename, PATH_MAX, "/VIDEO_TS/VTS_%02" PRIu16 "_%" PRIu16 ".VOB", vts_number, vob_number);

	mbs = ceil((dvd_vob.blocks * DVD_VIDEO_LB_LEN) / 1048576.0);
	dvd_vob.filesize_mbs = (uint64_t)mbs;

	return dvd_vob;

}
