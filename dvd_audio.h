#ifndef _DVD_AUDIO_H_
#define _DVD_AUDIO_H_

#include "dvd_track.h"

uint8_t dvd_track_audio_tracks(const ifo_handle_t *vts_ifo);

uint8_t dvd_audio_active_tracks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

uint8_t dvd_audio_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, const uint8_t audio_track);

uint8_t dvd_track_num_audio_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_track_has_audio_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

const char *dvd_audio_codec(const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

uint8_t dvd_audio_channels(const ifo_handle_t *vts_ifo, const uint8_t audio_track);

const char *dvd_audio_stream_id(const ifo_handle_t *vts_ifo, const uint8_t audio_track);

const char *dvd_audio_lang_code(const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

#endif
