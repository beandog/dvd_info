#ifndef DVD_INFO_SUBTITLES_H
#define DVD_INFO_SUBTITLES_H

#include "dvd_track.h"

struct dvd_subtitle {
	uint8_t track;
	uint8_t active;
	char stream_id[DVD_SUBTITLE_STREAM_ID + 1];
	char lang_code[DVD_SUBTITLE_LANG_CODE + 1];
};

uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo);

uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

uint8_t dvd_subtitle_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track);

uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_subtitle_lang_code(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t subtitle_track);

bool dvd_subtitle_stream_id(char *dest_str, const uint8_t subtitle_track);

#endif
