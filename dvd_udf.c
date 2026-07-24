#include "dvd_udf.h"

struct dvd_udf_file_t dvd_udf_file_open(dvd_reader_t *dvdread_dvd, char *udf_filename) {

	uint32_t dvdread_udf_block = 0;
	uint32_t dvdread_udf_size = 0;
	uint64_t filesize = 0;
	uint64_t filesize_mbs = 0;

	struct dvd_udf_file_t dvd_udf_file;

	memset(dvd_udf_file.filename, '\0', PATH_MAX);
	strncpy(dvd_udf_file.filename, udf_filename, PATH_MAX);
	dvd_udf_file.starting_block = 0;
	dvd_udf_file.blocks = 0;
	dvd_udf_file.filesize = 0;
	dvd_udf_file.filesize_mbs = 0;

	dvdread_udf_block = UDFFindFile(dvdread_dvd, udf_filename, &dvdread_udf_size);

	if(dvdread_udf_size > 0) {

		double mbs = 0;

		dvd_udf_file.starting_block = (uint64_t)dvdread_udf_block;

		dvd_udf_file.filesize = (uint64_t)dvdread_udf_size;
		dvd_udf_file.blocks = dvd_udf_file.filesize / DVD_VIDEO_LB_LEN;

		mbs = ceil((dvd_udf_file.blocks * DVD_VIDEO_LB_LEN) / 1048576.0);
		dvd_udf_file.filesize_mbs = (uint64_t)mbs;

	}

	return dvd_udf_file;

}
