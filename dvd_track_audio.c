#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_track_audio.h"

// Note: Remember that the language code is set in the IFO
// See dvdread/ifo_print.c for same functionality (error checking)
char *dvd_track_audio_lang_code(const ifo_handle_t *track_ifo, const int audio_track) {

	char lang_code[3] = {'\0'};
	unsigned char lang_type;
	audio_attr_t *audio_attr;

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[audio_track];
	lang_type = audio_attr->lang_type;

	if(lang_type != 1)
		return "";

	snprintf(lang_code, DVD_AUDIO_LANG_CODE, "%c%c", audio_attr->lang_code >> 8, audio_attr->lang_code & 0xff);
	return strndup(lang_code, DVD_AUDIO_LANG_CODE);

}

// FIXME see dvdread/ifo_print.c:196 for how it handles mpeg2ext with drc, no drc,
// same for lpcm; also a bug if it reports #5, and defaults to bug report if
// any above 6 (or under 0)
// FIXME check for multi channel extension
int dvd_track_audio_codec(const ifo_handle_t *track_ifo, const int audio_track, char *p) {

	unsigned char audio_codec;
	char *audio_codecs[7] = { "ac3", "?", "mpeg1", "mpeg2", "lpcm", "sdds", "dts" };
	audio_attr_t *audio_attr;

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[audio_track];
	audio_codec = audio_attr->audio_format;

	strncpy(p, audio_codecs[audio_codec], 5);

	return 0;

}

int dvd_track_audio_num_channels(const ifo_handle_t *track_ifo, const int audio_track) {

	unsigned char uc_num_channels;
	int num_channels;
	audio_attr_t *audio_attr;

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[audio_track];
	uc_num_channels = audio_attr->channels;
	num_channels = (int)uc_num_channels + 1;

	return num_channels;

}

int dvd_track_audio_stream_id(const ifo_handle_t *track_ifo, const int audio_track) {

	int audio_id[7] = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};
	unsigned char audio_format;
	int audio_stream_id = 0;
	audio_attr_t *audio_attr;

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[audio_track];
	audio_format = audio_attr->audio_format;
	audio_stream_id = audio_id[audio_format] + audio_track;

	return audio_stream_id;

}
