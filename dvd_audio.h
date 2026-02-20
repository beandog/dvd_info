#ifndef DVD_INFO_AUDIO_H
#define DVD_INFO_AUDIO_H

#include "dvd_track.h"

struct dvd_audio {
	uint8_t ix;
	uint8_t track;
	bool active;
	char stream_id[DVD_AUDIO_STREAM_ID + 1];
	char lang_code[DVD_AUDIO_LANG_CODE + 1];
	char quantization[DVD_AUDIO_QUANTIZATION + 1];
	char type[DVD_AUDIO_TYPE + 1];
	char codec[DVD_AUDIO_CODEC + 1];
	uint8_t channels;
};

uint8_t dvd_track_audio_tracks(ifo_handle_t *vts_ifo);

uint8_t dvd_audio_active_tracks(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t title_track);

bool dvd_audio_active(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t title_track, uint8_t audio_track);

bool dvd_audio_codec(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track);

uint8_t dvd_audio_channels(ifo_handle_t *vts_ifo, uint8_t audio_track);

bool dvd_audio_quantization(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track);

bool dvd_audio_type(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track);

bool dvd_audio_stream_id(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track);

bool dvd_audio_lang_code(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track);

bool dvd_track_has_audio_lang_code(ifo_handle_t *vts_ifo, char *lang_code);

#endif
