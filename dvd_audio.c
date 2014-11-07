#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_audio.h"

/** Audio Streams **/

/**
 * Get the number of audio streams for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @return number of audio streams
 */
uint8_t dvd_track_audio_tracks(const ifo_handle_t *vts_ifo) {

	uint8_t audio_streams = vts_ifo->vtsi_mat->nr_of_vts_audio_streams;

	if(audio_streams >= 0)
		return audio_streams;
	else
		return 0;

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
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @return number of PGC audio streams marked as active
 */
uint8_t dvd_audio_active_tracks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track) {

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;
	uint8_t idx = 0;
	uint8_t audio_tracks = 0;

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
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @param audio_track audio track
 * @return boolean
 */
uint8_t dvd_audio_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, const uint8_t audio_track) {

	uint8_t audio_tracks = dvd_track_audio_tracks(vts_ifo);

	if(audio_track > DVD_AUDIO_STREAM_LIMIT || audio_track > audio_tracks)
		return 0;

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;

	if(pgc->audio_control[audio_track - 1] & 0x8000)
		return 1;

	return 0;

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

		strncpy(str, dvd_audio_lang_code(vts_ifo, i), DVD_AUDIO_LANG_CODE);

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
