#include "dvd_audio.h"

/** Audio Tracks **/

/**
 * Get the number of audio streams for a track
 */
uint8_t dvd_track_audio_tracks(ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat == NULL)
		return 0;

	uint8_t audio_tracks = vts_ifo->vtsi_mat->nr_of_vts_audio_streams;

	return audio_tracks;

}

/**
 * Examine the PGC for the track IFO directly and see if there are any audio
 * control entries marked as active.  This is an alternative way of checking
 * for the number of audio streams, compared to looking at the VTS directly.
 * This is useful for debugging, and flushing out either badly mastered DVDs or
 * getting a closer identifier of how many streams this has.
 *
 * Some software uses this number of audio streams in the pgc instead of the
 * one in the VTSI MAT, such as mplayer and HandBrake, which will skip over
 * the other ones completely.
 */
uint8_t dvd_audio_active_tracks(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t title_track) {

	if(title_track == 0)
		return 0;

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return 0;

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;
	uint8_t idx = 0;
	uint8_t audio_tracks = 0;

	if(!pgc)
		return 0;

	for(idx = 0; idx < DVD_AUDIO_STREAM_LIMIT; idx++) {

		if(pgc->audio_control[idx] & 0x8000)
			audio_tracks++;

	}

	return audio_tracks;

}

/**
 * Look through the program chain to see if an audio track is flagged as
 * active or not.
 *
 * There is no fixed order to active, inactive tracks relative to each other.
 * The first can be inactive, the next active, etc.
 */
bool dvd_audio_active(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t title_track, uint8_t audio_track) {

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
 * Get the codec for an audio track.
 *
 * FIXME see dvdread/ifo_print.c:196 for how it handles mpeg2ext with drc, no drc,
 * same for lpcm; also a bug if it reports #5, and defaults to bug report if
 * any above 6 (or under 0)
 * FIXME check for multi channel extension
 *
 * Possible values: AC3, MPEG1, MPEG2, LPCM, SDDS, DTS
 */
bool dvd_audio_codec(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track) {

	if(vts_ifo->vtsi_mat == NULL)
		return false;

	char *audio_codecs[7] = { "ac3", "", "mpeg1", "mpeg2", "lpcm", "sdds", "dts" };
	audio_attr_t *audio_attr =  &vts_ifo->vtsi_mat->vts_audio_attr[audio_track];
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
 */
uint8_t dvd_audio_channels(ifo_handle_t *vts_ifo, uint8_t audio_track) {

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
 */
bool dvd_audio_stream_id(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track) {

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
 */
bool dvd_audio_lang_code(char *dest_str, ifo_handle_t *vts_ifo, uint8_t audio_track) {

	if(vts_ifo->vtsi_mat == NULL)
		return false;

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_track];
	uint8_t lang_type = audio_attr->lang_type;

	if(lang_type != 1)
		return true;

	snprintf(dest_str, DVD_AUDIO_LANG_CODE + 1, "%c%c", audio_attr->lang_code >> 8, audio_attr->lang_code & 0xff);

	if(!isalpha(dest_str[0]) || !isalpha(dest_str[1])) {
		dest_str[0] = '\0';
		dest_str[1] = '\0';
	}

	return true;

}
