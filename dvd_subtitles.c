#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_subtitles.h"

/** Subtitles **/

/**
 * Get the number of subtitle streams for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @return number of subtitles
 */
uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo) {

	uint8_t subtitles = vts_ifo->vtsi_mat->nr_of_vts_subp_streams;

	if(subtitles >= 0)
		return subtitles;
	else
		return 0;

}

/**
 * Get the number of subtitle streams marked as active.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @return number of active subtitles
 */
uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;
	uint8_t idx = 0;
	uint8_t active_subtitles = 0;

	for(idx = 0; idx < DVD_SUBTITLE_STREAM_LIMIT; idx++) {

		if(pgc->subp_control[idx] & 0x80000000)
			active_subtitles++;

	}

	return active_subtitles;

}

/**
 * Check if a subtitle stream is flagged as active or not.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @param subtitle_track track number
 * @return boolean
 */
uint8_t dvd_subtitle_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track) {

	uint8_t subtitles = dvd_track_subtitles(vts_ifo);

	if(subtitle_track > DVD_SUBTITLE_STREAM_LIMIT || subtitle_track > subtitles)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;

	if(pgc->subp_control[subtitle_track - 1] & 0x80000000)
		return 1;

	return 0;

}

/**
 * Get the number of subtitle streams for a specific language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return number of subtitles
 */
uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code) {

	uint8_t streams = dvd_track_subtitles(vts_ifo);

	if(streams == 0)
		return 0;

	uint8_t matches = 0;
	uint8_t i = 0;
	char str[DVD_SUBTITLE_LANG_CODE + 1] = {'\0'};

	for(i = 0; i < streams; i++) {

		strncpy(str, dvd_subtitle_lang_code(vts_ifo, i), DVD_SUBTITLE_LANG_CODE);

		if(strncmp(str, lang_code, DVD_SUBTITLE_LANG_CODE) == 0)
			matches++;

	}

	return matches;

}

/**
 * Check if a DVD track has a specific subtitle language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return boolean
 */
bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code) {

	if(dvd_track_num_subtitle_lang_code_streams(vts_ifo, lang_code) > 0)
		return true;
	else
		return false;

}
