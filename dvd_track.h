#ifndef DVD_INFO_TRACK_H
#define DVD_INFO_TRACK_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "dvd_cell.h"
#include "dvd_vmg_ifo.h"
#include "dvd_time.h"

struct dvd_video {
	char codec[DVD_VIDEO_CODEC + 1];
	char format[DVD_VIDEO_FORMAT + 1];
	char aspect_ratio[DVD_VIDEO_ASPECT_RATIO + 1];
	uint16_t width;
	uint16_t height;
	bool letterbox;
	bool pan_and_scan;
	uint8_t df;
	char fps[DVD_VIDEO_FPS + 1];
	uint8_t angles;
};

struct dvd_track {
	uint16_t track;
	bool valid;
	uint16_t vts;
	uint8_t ttn;
	uint16_t ptts;
	char length[DVD_TRACK_LENGTH + 1];
	uint32_t msecs;
	uint8_t chapters;
	uint8_t audio_tracks;
	uint8_t subtitles;
	uint8_t cells;
	bool min_sector_error;
	bool max_sector_error;
	bool repeat_first_sector_error;
	bool repeat_last_sector_error;
	struct dvd_video dvd_video;
	struct dvd_audio *dvd_audio_tracks;
	uint8_t active_audio_streams;
	struct dvd_subtitle *dvd_subtitles;
	uint8_t active_subs;
	struct dvd_chapter *dvd_chapters;
	struct dvd_cell *dvd_cells;
	uint64_t blocks;
	uint64_t filesize;
	double filesize_mbs;
};


uint16_t dvd_vts_ifo_number(ifo_handle_t *vmg_ifo, uint16_t track_number);

uint8_t dvd_track_ttn(ifo_handle_t *vmg_ifo, uint16_t track_number);

uint16_t dvd_track_title_parts(ifo_handle_t *vmg_ifo, uint16_t track_number);

bool dvd_vts_id(char *dest_str, ifo_handle_t *vts_ifo);

uint8_t dvd_track_chapters(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

uint8_t dvd_track_cells(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

uint64_t dvd_track_blocks(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

uint64_t dvd_track_filesize(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

double dvd_track_filesize_mbs(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

#endif
