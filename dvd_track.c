#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_video.h"
#include "dvd_time.h"

/**
 * Functions used to get information about a DVD track
 */

/**
 * Check if the IFO is a VTS or not.
 *
 * libdvdread populates the ifo_handle with various data, but the structure is
 * the same for both a VMG IFO and a VTS one.  This does a few checks to make
 * sure that the ifo_handle passed in is a Video Title Set.
 *
 * @param ifo dvdread IFO handle
 * @return boolean
 */
bool ifo_is_vts(const ifo_handle_t *ifo) {

	if(ifo->vtsi_mat == NULL)
		return false;
	else
		return true;

}

/**
 * Get the IFO number that a track resides in
 *
 * @param vmg_ifo dvdread handler for primary IFO
 * @return IFO number
 */
uint16_t dvd_vts_ifo_number(const ifo_handle_t *vmg_ifo, const uint16_t track_number) {

	// TODO research
	// Should these be the same number
	// vts_ttn = vmg_ifo->tt_srpt->title[title_track_idx].vts_ttn;

	uint16_t ifo_number = vmg_ifo->tt_srpt->title[track_number - 1].title_set_nr;

	if(ifo_number >= 0)
		return ifo_number;
	else
		return 1;

}

uint8_t dvd_track_ttn(const ifo_handle_t *vmg_ifo, const uint16_t track_number) {

	uint8_t ttn = vmg_ifo->tt_srpt->title[track_number - 1].vts_ttn;

	if(ttn > 0)
		return ttn;
	else
		return 1;

}

/**
 * Get the track's VTS id
 * Possible that it's blank, usually set to DVDVIDEO-VTS otherwise.
 *
 * @param vts_ifo libdvdread IFO handle
 */
const char *dvd_vts_id(const ifo_handle_t *vts_ifo) {

	size_t i = 0;

	for(i = 0; i < strlen(vts_ifo->vtsi_mat->vts_identifier); i++) {
		if(!isascii(vts_ifo->vtsi_mat->vts_identifier[i]))
			return "";
	}

	return strndup(vts_ifo->vtsi_mat->vts_identifier, DVD_VTS_ID);

}

uint8_t dvd_track_chapters(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	// If there's no cell playback, then override the number in the PGC and report as
	// zero so that they are not accessed.
	if(pgc->cell_playback == NULL)
		return 0;

	uint8_t chapters = pgc->nr_of_programs;

	if(chapters > 0)
		return chapters;
	else
		return 0;

}

uint8_t dvd_track_cells(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	// If there's no cell playback, then override the number in the PGC and report as
	// zero so that they are not accessed.
	if(pgc->cell_playback == NULL)
		return 0;

	uint8_t cells = pgc->nr_of_cells;

	if(cells >= 0)
		return cells;
	else
		return 0;

}

// It's a *safe guess* that if the program_map is NULL or there is no starting cell,
// that the starting cell is actually the same as the chapter number.  See DVD id 79,
// track #12 for an example where this matches (NULL values)
//
// TODO some closer examination to check that cells and chapters match up properly
// is probably in order.  Some scenarios to look for would be where there are more
// chapters than cells, and the lengths of chapters don't exceed cells either.
uint8_t dvd_chapter_startcell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;

	if(pgc->program_map == NULL)
		return chapter_number;

	uint8_t startcell = pgc->program_map[chapter_number - 1];

	if(startcell > 0)
		return startcell;
	else
		return chapter_number;

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
const char *dvd_audio_lang_code(const ifo_handle_t *vts_ifo, const uint8_t audio_stream) {

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_stream];
	uint8_t lang_type = audio_attr->lang_type;

	if(lang_type != 1)
		return "";

	char lang_code[3] = {'\0'};

	snprintf(lang_code, DVD_AUDIO_LANG_CODE + 1, "%c%c", audio_attr->lang_code >> 8, audio_attr->lang_code & 0xff);

	return strndup(lang_code, DVD_AUDIO_LANG_CODE);

}

// Have dvd_debug check for issues here.
// FIXME I want to have some kind of distinguishment in here, and for audio tracks
// if it's an invalid language.  If it's missing one, set it to unknown (for example)
// but if it's invalid, maybe guess that it's in English, or something?  Dunno.
// Having a best-guess approach might not be bad, maybe even look at region codes
/**
 * Get the lang code of a subtitle track for a title track
 *
 * @param vts_ifo dvdread track IFO handler
 * @param subtitle_track subtitle track number
 * @retval lang code
 */
const char *dvd_subtitle_lang_code(const ifo_handle_t *vts_ifo, const uint8_t subtitle_track) {

	char lang_code[3] = {'\0'};
	subp_attr_t *subp_attr = NULL;

	subp_attr = &vts_ifo->vtsi_mat->vts_subp_attr[subtitle_track];

	if(subp_attr->type == 0 && subp_attr->lang_code == 0 && subp_attr->zero1 == 0 && subp_attr->zero2 == 0 && subp_attr->lang_extension == 0) {
		return "";
	}
	snprintf(lang_code, DVD_SUBTITLE_LANG_CODE + 1, "%c%c", subp_attr->lang_code >> 8, subp_attr->lang_code & 0xff);

	if(!isalpha(lang_code[0]) || !isalpha(lang_code[1]))
		return "";

	return strndup(lang_code, DVD_SUBTITLE_LANG_CODE);

}

/**
 * Get the stream ID for a subtitle, an index that starts at 0x20
 *
 * This is only here for lsdvd output compatability.  The function just adds
 * the index to 0x20.
 *
 * @param subtitle_track subtitle track number
 * @return stream id
 */
const char *dvd_subtitle_stream_id(const uint8_t subtitle_track) {

	char str[DVD_SUBTITLE_STREAM_ID + 1] = {'\0'};

	snprintf(str, DVD_SUBTITLE_STREAM_ID + 1, "0x%x", 0x20 + subtitle_track);

	return strndup(str, DVD_SUBTITLE_STREAM_ID);

}
