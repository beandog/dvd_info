#include "dvd_audio.h"

/** Audio Streams **/

/**
 * Look through the program chain to see if an audio track is flagged as
 * active or not.
 *
 * There is no fixed order to active, inactive tracks relative to each other.
 * The first can be inactive, the next active, etc.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @param audio_track audio track
 * @return boolean
 */
bool dvd_audio_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, const uint8_t audio_track) {

	if(title_track == 0)
		return false;

	uint8_t audio_tracks = dvd_track_audio_tracks(vts_ifo);

	if(audio_track > DVD_AUDIO_STREAM_LIMIT || audio_track > audio_tracks)
		return false;

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return false;

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;

	if(!pgc)
		return false;

	if(pgc->audio_control[audio_track] & 0x8000)
		return true;

	return false;

}

/**
 * Get the number of audio streams for a specific language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return number of subtitles
 */
uint8_t dvd_track_num_audio_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code) {

	uint8_t audio_tracks = dvd_track_audio_tracks(vts_ifo);
	uint8_t language_tracks = 0;
	char str[DVD_AUDIO_LANG_CODE + 1] = {'\0'};
	uint8_t i = 0;

	for(i = 0; i < audio_tracks; i++) {

		dvd_audio_lang_code(str, vts_ifo, i);

		if(strncmp(str, lang_code, DVD_AUDIO_LANG_CODE) == 0)
			language_tracks++;

	}

	return language_tracks;

}

/**
 * Check if a DVD track has a specific audio language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return boolean
 */
bool dvd_track_has_audio_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code) {

	if(dvd_track_num_audio_lang_code_streams(vts_ifo, lang_code) > 0)
		return true;
	else
		return false;

}

/**
 * Get the codec for an audio track.
 *
 * FIXME see dvdread/ifo_print.c:196 for how it handles mpeg2ext with drc, no drc,
 * same for lpcm; also a bug if it reports #5, and defaults to bug report if
 * any above 6 (or under 0)
 * FIXME check for multi channel extension
 *
 * Possible values: AC3, MPEG1, MPEG2, LPCM, SDDS, DTS
 * @param vts_ifo dvdread track IFO handler
 * @param audio_stream audio track number
 * @return audio codec
 */
bool dvd_audio_codec(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t audio_stream) {

	if(vts_ifo->vtsi_mat == NULL)
		return false;

	const char *audio_codecs[7] = { "ac3", "", "mpeg1", "mpeg2", "lpcm", "sdds", "dts" };
	audio_attr_t *audio_attr =  &vts_ifo->vtsi_mat->vts_audio_attr[audio_stream];
	uint8_t audio_codec = audio_attr->audio_format;

	strncpy(dest_str, audio_codecs[audio_codec], DVD_AUDIO_CODEC);
	return true;

}

/**
 * Get the number of channels for an audio track
 *
 * Human-friendly interpretation of what channels are:
 * 1: mono
 * 2: stereo
 * 3: 2.1, stereo and subwoofer
 * 4: front right, front left, rear right, rear left
 * 5: front right, front left, rear right, rear left, subwoofer
 *
 * @param vts_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @return num channels
 */
uint8_t dvd_audio_channels(const ifo_handle_t *vts_ifo, const uint8_t audio_track) {

	if(vts_ifo->vtsi_mat == NULL)
		return 0;

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_track];
	uint8_t channels = audio_attr->channels + 1;

	return channels;

}

/**
 * Get the stream ID for an audio track
 *
 * AC3 = 0x80 to 0x87
 * DTS = 0x88 to 0x8f
 * LPCM = 0xa0 to 0xa7
 *
 * @param vts_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @return audio stream id
 */
bool dvd_audio_stream_id(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t audio_track) {

	if(vts_ifo->vtsi_mat == NULL)
		return false;

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_track];
	uint8_t audio_format = audio_attr->audio_format;
	uint8_t audio_id[7] = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};
	uint8_t audio_stream_id = audio_id[audio_format] + audio_track;

	snprintf(dest_str, DVD_AUDIO_STREAM_ID + 1, "0x%x", audio_stream_id);

	return true;

}

/**
 * Get the audio language code for a track.  A two-character string that is a
 * short name for a language.
 *
 * Note: Remember that the language code is set in the IFO
 * See dvdread/ifo_print.c for same functionality (error checking)
 *
 * DVD specification says that there is an ISO-639 character code here:
 * - http://stnsoft.com/DVD/ifo_vts.html
 * - http://www.loc.gov/standards/iso639-2/php/code_list.php
 * lsdvd uses 'und' (639-2) for undetermined if the lang_code and
 * lang_extension are both 0.
 *
 * Examples: en: English, fr: French, es: Spanish
 *
 * @param vts_ifo dvdread track IFO handler
 * @param audio_stream audio track number
 * @return language code
 */
bool dvd_audio_lang_code(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t audio_stream) {

	if(vts_ifo->vtsi_mat == NULL)
		return false;

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_stream];
	uint8_t lang_type = audio_attr->lang_type;

	if(lang_type != 1)
		return true;

	snprintf(dest_str, DVD_AUDIO_LANG_CODE + 1, "%c%c", audio_attr->lang_code >> 8, audio_attr->lang_code & 0xff);

	return true;

}

dvd_audio_t *dvd_audio_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t audio_track_ix) {

	dvd_audio_t *dvd_audio = calloc(1, sizeof(dvd_audio_t));

	if(dvd_audio == NULL || vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo) || vts_ifo == NULL)
		return NULL;

	memset(dvd_audio->stream_id, '\0', sizeof(dvd_audio->stream_id));
	dvd_audio_stream_id(dvd_audio->stream_id, vts_ifo, audio_track_ix);

	memset(dvd_audio->lang_code, '\0', sizeof(dvd_audio->lang_code));
	dvd_audio_lang_code(dvd_audio->lang_code, vts_ifo, audio_track_ix);

	memset(dvd_audio->codec, '\0', sizeof(dvd_audio->codec));
	dvd_audio_codec(dvd_audio->codec, vts_ifo, audio_track_ix);

	dvd_audio->active = dvd_audio_active(vmg_ifo, vts_ifo, track_number, audio_track_ix);

	dvd_audio->channels = dvd_audio_channels(vts_ifo, audio_track_ix);

	return dvd_audio;

}
