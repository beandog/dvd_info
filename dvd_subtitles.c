#include "dvd_subtitles.h"

/** Subtitles **/

/**
 * Check if a subtitle stream is flagged as active or not.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @param subtitle_track track number
 * @return boolean
 */
bool dvd_subtitle_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track) {

	if(vts_ifo->vts_pgcit == NULL || vts_ifo->vts_ptt_srpt == NULL || vts_ifo->vts_ptt_srpt->title == NULL)
		return false;

	uint8_t subtitles = dvd_track_subtitles(vts_ifo);

	if(subtitle_track > DVD_SUBTITLE_STREAM_LIMIT || subtitle_track > subtitles)
		return false;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;

	if(!pgc)
		return false;

	if(pgc->subp_control[subtitle_track - 1] & 0x80000000)
		return true;

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

		dvd_subtitle_lang_code(str, vts_ifo, i);

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

// Have dvd_debug check for issues here.
// FIXME I want to have some kind of distinguishment in here, and for audio tracks
// if it's an invalid language.  If it's missing one, set it to unknown (for example)
// but if it's invalid, maybe guess that it's in English, or something?  Dunno.
// Having a best-guess approach might not be bad, maybe even look at region codes
/**
 * Get the lang code of a subtitle track for a title track
 *
 */
void dvd_subtitle_lang_code(char *dest_str, const ifo_handle_t *vts_ifo, const uint8_t subtitle_track) {

	char lang_code[3] = {'\0'};
	subp_attr_t *subp_attr = NULL;

	subp_attr = &vts_ifo->vtsi_mat->vts_subp_attr[subtitle_track];

	if(subp_attr->type == 0 && subp_attr->lang_code == 0 && subp_attr->zero1 == 0 && subp_attr->zero2 == 0 && subp_attr->lang_extension == 0) {
		return;
	}
	snprintf(lang_code, DVD_SUBTITLE_LANG_CODE + 1, "%c%c", subp_attr->lang_code >> 8, subp_attr->lang_code & 0xff);

	if(!isalpha(lang_code[0]) || !isalpha(lang_code[1]))
		return;

	strncpy(dest_str, lang_code, DVD_SUBTITLE_LANG_CODE);

}

/**
 * Get the stream ID for a subtitle, an index that starts at 0x20
 *
 * This is only here for lsdvd output compatability.  The function just adds
 * the index to 0x20.
 *
 * Possible ranges: 0x20 to 0x3f
 *
 * @param subtitle_track subtitle track number
 */
void dvd_subtitle_stream_id(char *dest_str, const uint8_t subtitle_track) {

	snprintf(dest_str, DVD_SUBTITLE_STREAM_ID + 1, "0x%x", 0x20 + subtitle_track);

}

dvd_subtitle_t *dvd_subtitle_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number, uint8_t subtitle_track_ix) {

	if(vmg_ifo == NULL || !ifo_is_vmg(vmg_ifo) || vts_ifo == NULL)
		return NULL;

	dvd_subtitle_t *dvd_subtitle = calloc(1, sizeof(dvd_subtitle_t));

	if(dvd_subtitle == NULL)
		return NULL;

	dvd_subtitle->track = subtitle_track_ix + 1;
	dvd_subtitle->active = dvd_subtitle_active(vmg_ifo, vts_ifo, track_number, subtitle_track_ix);
	dvd_subtitle_lang_code(dvd_subtitle->lang_code, vts_ifo, subtitle_track_ix);
	dvd_subtitle_stream_id(dvd_subtitle->stream_id, subtitle_track_ix);

	return dvd_subtitle;

}
