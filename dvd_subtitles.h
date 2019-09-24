#ifndef DVD_INFO_SUBTITLES_H
#define DVD_INFO_SUBTITLES_H

#include "dvd_track.h"

struct dvd_subtitle {
	uint8_t track;
	bool active;
	char stream_id[DVD_SUBTITLE_STREAM_ID + 1];
	char lang_code[DVD_SUBTITLE_LANG_CODE + 1];
};

uint8_t dvd_track_subtitles(ifo_handle_t *vts_ifo);

uint8_t dvd_track_active_subtitles(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t title_track);

bool dvd_subtitle_active(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t title_track, uint8_t subtitle_track);

uint8_t dvd_track_num_subtitle_lang_code_streams(ifo_handle_t *vts_ifo, char *lang_code);

bool dvd_track_has_subtitle_lang_code(ifo_handle_t *vts_ifo, char *lang_code);

void dvd_subtitle_lang_code(char *dest_str, ifo_handle_t *vts_ifo, uint8_t subtitle_track);

void dvd_subtitle_stream_id(char *dest_str, uint8_t subtitle_track);

#endif
