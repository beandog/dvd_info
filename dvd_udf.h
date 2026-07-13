#ifndef DVD_INFO_UDF_H
#define DVD_INFO_UDF_H

#include <string.h>
#include <math.h>
#include <dvdread/dvd_udf.h>
#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif

// Filename must have prefix of "/VIDEO_TS/" with the leading slash
// For example: "/VIDEO_TS/VTS_01_1.VOB"
struct dvd_udf_file_t {
	char filename[PATH_MAX];
	uint64_t filesize;
	uint64_t filesize_mbs;
	uint64_t blocks;
	uint64_t starting_block;
};

struct dvd_udf_file_t dvd_udf_file_open(dvd_reader_t *dvdread_dvd, char *udf_filename);

#endif
