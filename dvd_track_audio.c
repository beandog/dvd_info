#include "dvd_track_audio.h"

// Note: Remember that the language code is set in the IFO
int dvd_track_audio_lang_code(const ifo_handle_t *track_ifo, const int audio_track, char *p) {

	char lang_code[5] = {'\0'};
	unsigned char lang_type;
	audio_attr_t *audio_attr;

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[audio_track];
	lang_type = audio_attr->lang_type;

	if(lang_type == 0) {

		// See dvdread/ifo_print.c for same functionality (error checking)
		if(audio_attr->lang_code != 0 && audio_attr->lang_code != 0xffff) {

			fprintf(stderr, "libdvdread bug? lang_code: 0x%x\n", audio_attr->lang_code);
			return 1;

		}

		strncpy(p, "xx", 3);

	} else if(lang_type == 1) {

		sprintf(lang_code, "%c%c", audio_attr->lang_code>>8, audio_attr->lang_code & 0xff);
		strncpy(p, lang_code, 5);

	} else {

		// See dvdread/ifo_print.c for same functionality (error checking)
		fprintf(stderr, "libdvdread bug? Unknown lang code: 0x%x\n", audio_attr->lang_code);
		strncpy(p, "", 2);

		return 1;

	}

	return 0;

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
