#ifndef _DVD_TRACK_H_
#define _DVD_TRACK_H_

bool ifo_is_vts(const ifo_handle_t *ifo);

uint16_t dvd_vts_ifo_number(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

uint8_t dvd_track_ttn(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

const char *dvd_vts_id(const ifo_handle_t *vts_ifo);

uint8_t dvd_track_audio_tracks(const ifo_handle_t *vts_ifo);

uint8_t dvd_audio_active_tracks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

uint8_t dvd_audio_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, const uint8_t audio_track);

uint8_t dvd_track_num_audio_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_track_has_audio_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

/** Subtitles **/

uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo);

uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

uint8_t dvd_subtitle_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track);

uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

uint8_t dvd_track_chapters(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

uint8_t dvd_track_cells(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

const char *dvd_audio_lang_code(const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

uint8_t dvd_chapter_startcell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

const char *dvd_audio_codec(const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

uint8_t dvd_audio_channels(const ifo_handle_t *vts_ifo, const uint8_t audio_track);

const char *dvd_audio_stream_id(const ifo_handle_t *vts_ifo, const uint8_t audio_track);

const char *dvd_subtitle_lang_code(const ifo_handle_t *vts_ifo, const uint8_t subtitle_track);

const char *dvd_subtitle_stream_id(const uint8_t subtitle_track);

// _DVD_TRACK_H_
#endif
