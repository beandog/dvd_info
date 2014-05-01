#include "dvd_track_audio.h"

int dvd_track_audio_lang_code(const ifo_handle_t *track_ifo, const int audio_track, char *p) {

	char lang_code[5] = {'\0'};
	audio_attr_t *audio_attr;

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[audio_track];

	sprintf(lang_code, "%c%c", audio_attr->lang_code>>8, audio_attr->lang_code & 0xff);
	strncpy(p, lang_code, 5);

	return 0;

}

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
