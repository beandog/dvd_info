#ifndef DVD_INFO_AUDIO_H
#define DVD_INFO_AUDIO_H

#include "dvd_track.h"

struct dvd_audio {
	uint8_t ix;
	uint8_t track;
	bool active;
	char stream_id[DVD_AUDIO_STREAM_ID + 1];
	char lang_code[DVD_AUDIO_LANG_CODE + 1];
	char codec[DVD_AUDIO_CODEC + 1];
	uint8_t channels;
};

typedef struct {
	uint8_t ix;
	uint8_t track;
	bool active;
	char stream_id[DVD_AUDIO_STREAM_ID + 1];
	char lang_code[DVD_AUDIO_LANG_CODE + 1];
	char codec[DVD_AUDIO_CODEC + 1];
	uint8_t channels;
} dvd_audio_t;

bool dvd_audio_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, const uint8_t audio_track);

uint8_t dvd_track_num_audio_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_track_has_audio_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

bool dvd_audio_codec(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

uint8_t dvd_audio_channels(const ifo_handle_t *vts_ifo, const uint8_t audio_track);

bool dvd_audio_stream_id(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t audio_track);

bool dvd_audio_lang_code(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

dvd_audio_t *dvd_audio_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t audio_track);

#endif
