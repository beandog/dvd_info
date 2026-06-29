#include "dvd_udf.h"

struct dvd_udf_file_t dvd_udf_file_open(dvd_reader_t *dvdread_dvd, char *udf_filename) {

	uint32_t block = 0;
	uint32_t bytes = 0;
	uint32_t mbs = 0;

	struct dvd_udf_file_t dvd_udf_file;

	memset(dvd_udf_file.filename, '\0', DVD_UDF_PATH_MAX + 1);
	strncpy(dvd_udf_file.filename, udf_filename, DVD_UDF_PATH_MAX + 1);
	dvd_udf_file.starting_block = 0;
	dvd_udf_file.bytes = 0;
	dvd_udf_file.mbs = 0;

	block = UDFFindFile(dvdread_dvd, udf_filename, &bytes);

	if(block > 0) {

		dvd_udf_file.starting_block = block;
		dvd_udf_file.bytes = bytes;
		if(bytes > 0)
			dvd_udf_file.mbs = (uint32_t)(bytes / 1048576.0) + 1;

	}

	return dvd_udf_file;

}
