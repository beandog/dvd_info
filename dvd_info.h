#ifndef DVD_INFO_H
#define DVD_INFO_H

#include <stdint.h>
#include <unistd.h>
#include "dvd_specs.h"

struct dvd_info {
	uint8_t valid;
	char dvdread_id[DVD_DVDREAD_ID + 1];
	uint16_t video_title_sets;
	uint16_t valid_video_title_sets;
	uint16_t invalid_video_title_sets;
	uint8_t side;
	char title[DVD_TITLE + 1];
	char provider_id[DVD_PROVIDER_ID + 1];
	char vmg_id[DVD_VMG_ID + 1];
	uint16_t tracks;
	uint16_t longest_track;
	uint16_t valid_tracks;
	uint16_t invalid_tracks;
};

#endif
