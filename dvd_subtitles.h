#ifndef DVD_INFO_SUBTITLES_H
#define DVD_INFO_SUBTITLES_H

#include "dvd_track.h"

struct dvd_subtitle {
	uint8_t track;
	bool active;
	char stream_id[DVD_SUBTITLE_STREAM_ID + 1];
	char lang_code[DVD_SUBTITLE_LANG_CODE + 1];
};

typedef struct {
	uint8_t track;
	bool active;
	char stream_id[DVD_SUBTITLE_STREAM_ID + 1];
	char lang_code[DVD_SUBTITLE_LANG_CODE + 1];
} dvd_subtitle_t;

bool dvd_subtitle_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track);

uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

void dvd_subtitle_lang_code(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t subtitle_track);

void dvd_subtitle_stream_id(char *dest_str, const uint8_t subtitle_track);

dvd_subtitle_t *dvd_subtitle_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t subtitle_track);

#endif
