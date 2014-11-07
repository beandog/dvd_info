#ifndef _DVD_SUBTITLES_H_
#define _DVD_SUBTITLES_H_

uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo);

uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

uint8_t dvd_subtitle_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track);

uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

const char *dvd_subtitle_lang_code(const ifo_handle_t *vts_ifo, const uint8_t subtitle_track);

const char *dvd_subtitle_stream_id(const uint8_t subtitle_track);

#endif
