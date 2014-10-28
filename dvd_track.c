#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"
#include "dvd_time.h"

bool ifo_is_vts(const ifo_handle_t *ifo) {

	if(ifo->vtsi_mat == NULL)
		return false;
	else
		return true;

}

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

uint8_t dvd_video_angles(const ifo_handle_t *vmg_ifo, const uint16_t track_number) {

	uint8_t angles = vmg_ifo->tt_srpt->title[track_number - 1].nr_of_angles;

	if(angles >= 0)
		return angles;
	else
		return 0;

}

const char *dvd_vts_id(const ifo_handle_t *vts_ifo) {

	size_t i = 0;

	for(i = 0; i < strlen(vts_ifo->vtsi_mat->vts_identifier); i++) {
		if(!isascii(vts_ifo->vtsi_mat->vts_identifier[i]))
			return "";
	}

	return strndup(vts_ifo->vtsi_mat->vts_identifier, DVD_VTS_ID);

}

/**
 * MPEG version
 * ifo->vtsi_mat->vts_video_attr.mpeg_version
 *
 * 0 = MPEG1
 * 1 = MPEG2
 * 2 = Reserved, do not use
 * 3 = Reserved, do not use
 */
uint8_t dvd_track_mpeg_version(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		return 1;
	else if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return 2;
	else
		return 0;

}

bool dvd_track_mpeg1(const ifo_handle_t *vts_ifo) {

	if(dvd_track_mpeg_version(vts_ifo) == 1)
		return true;
	else
		return false;

}

bool dvd_track_mpeg2(const ifo_handle_t *vts_ifo) {

	if(dvd_track_mpeg_version(vts_ifo) == 2)
		return true;
	else
		return false;

}

/*
 * Standard video format
 * ifo->vtsi_mat->vts_video_attr.video_format
 *
 * 0 = NTSC
 * 1 = PAL
 * 2 = Reserved, do not use
 * 3 = Reserved, do not use
 */

bool dvd_track_ntsc_video(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.video_format == 0)
		return true;
	else
		return false;

}

bool dvd_track_pal_video(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.video_format == 1)
		return true;
	else
		return false;

}

uint16_t dvd_video_height(const ifo_handle_t *vts_ifo) {

	int video_height = 0;
	uint8_t picture_size = vts_ifo->vtsi_mat->vts_video_attr.picture_size;

	if(dvd_track_ntsc_video(vts_ifo))
		video_height = 480;
	else if(dvd_track_pal_video(vts_ifo))
		video_height = 576;

	if(picture_size == 3 && video_height > 0) {
		video_height = video_height / 2;
		if(video_height < 0)
			video_height = 0;
	}

	return (uint16_t)video_height;

}

uint16_t dvd_video_width(const ifo_handle_t *vts_ifo) {

	uint16_t video_width = 0;
	uint16_t video_height = dvd_video_height(vts_ifo);
	uint8_t picture_size = vts_ifo->vtsi_mat->vts_video_attr.picture_size;

	if(video_height == 0)
		return 0;

	if(picture_size == 0) {
		video_width = 720;
	} else if(picture_size == 1) {
		video_width = 704;
	} else if(picture_size == 2) {
		video_width = 352;
	} else if(picture_size == 3) {
		video_width = 352;
	} else {
		// Catch wrong integer values burned in DVD, and guess at the width
		video_width = 720;
	}

	return video_width;

}

bool dvd_track_valid_aspect_ratio(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0 || vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return true;
	else
		return false;

}

bool dvd_track_aspect_ratio_4x3(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
		return true;
	else
		return false;

}

bool dvd_track_aspect_ratio_16x9(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return true;
	else
		return false;

}

uint8_t dvd_video_df(const ifo_handle_t *vts_ifo) {

	uint8_t permitted_df = vts_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df >= 0)
		return permitted_df;
	else
		return 0;

}

bool dvd_video_letterbox(const ifo_handle_t *vts_ifo) {

	uint8_t permitted_df = vts_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df == 0 || permitted_df == 2)
		return true;
	else
		return false;

}

bool dvd_video_pan_scan(const ifo_handle_t *vts_ifo) {

	uint8_t permitted_df = vts_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df == 0 || permitted_df == 1)
		return true;
	else
		return false;

}

const char *dvd_video_codec(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		return "MPEG1";
	else if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return "MPEG2";
	else
		return "";

}

const char *dvd_track_video_format(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.video_format == 0)
		return "NTSC";
	else if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return  "PAL";
	else
		return "";

}

const char *dvd_video_aspect_ratio(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
		return "4:3";
	else if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return "16:9";
	else
		return "";

}

double dvd_track_fps(dvd_time_t *dvd_time) {

	double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};
	double fps = frames_per_s[(dvd_time->frame_u & 0xc0) >> 6];

	return fps;

}

const char *dvd_track_str_fps(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number) {

	char str[DVD_VIDEO_FPS + 1] = {'\0'};
	uint8_t ttn = dvd_track_ttn(vmg_ifo, track_number);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;
	double fps = dvd_track_fps(&pgc->playback_time);

	if(fps > 0) {
		snprintf(str, DVD_VIDEO_FPS + 1, "%02.02f", fps);
		return strndup(str, DVD_VIDEO_FPS);
	} else {
		return "";
	}

}

/** Audio **/

uint8_t dvd_track_audio_tracks(const ifo_handle_t *vts_ifo) {

	uint8_t audio_streams = vts_ifo->vtsi_mat->nr_of_vts_audio_streams;

	if(audio_streams >= 0)
		return audio_streams;
	else
		return 0;

}

// Used by dvd_debug
// Looks at the program control chain instead of the VTS for number of audio streams
// This should eliminate showing ghost audio streams that don't actually exist
// See https://github.com/thierer/lsdvd/commit/2adcc7d8ab1d3ccaa8b2aa294ad15ba5f72533d4
// See http://dvdnav.mplayerhq.hu/dvdinfo/pgc.html
// Looks for whether a stream available flag is set for each audio stream
uint8_t dvd_audio_active_tracks(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track) {

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;
	uint8_t idx = 0;
	uint8_t i = 0;

	for(idx = 0; idx < DVD_AUDIO_STREAM_LIMIT; idx++) {

		if(pgc->audio_control[idx] & 0x8000)
			i++;

	}

	return i;

}

uint8_t dvd_audio_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, const uint8_t audio_track) {

	if(audio_track > DVD_AUDIO_STREAM_LIMIT || audio_track > dvd_audio_active_tracks(vmg_ifo, vts_ifo, title_track))
		return 0;

	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;

	if(pgc->audio_control[audio_track - 1] & 0x8000)
		return 1;

	return 0;

}

uint8_t dvd_track_num_audio_lang_code_streams(const ifo_handle_t *vts_ifo, const char *p) {

	uint8_t num_track_audio_streams = dvd_track_audio_tracks(vts_ifo);
	uint8_t num_lang_streams = 0;
	char lang_code[DVD_AUDIO_LANG_CODE + 1] = {'\0'};
	uint8_t i = 0;

	for(i = 0; i < num_track_audio_streams; i++) {

		strncpy(lang_code, dvd_audio_lang_code(vts_ifo, i), DVD_AUDIO_LANG_CODE);

		if(strncmp(lang_code, p, DVD_AUDIO_LANG_CODE) == 0) {
			num_lang_streams++;
		}

	}

	return num_lang_streams;

}

bool dvd_track_has_audio_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code) {

	if(dvd_track_num_audio_lang_code_streams(vts_ifo, lang_code) > 0)
		return true;
	else
		return false;

}


/** Subtitles **/

uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo) {

	uint8_t subtitles = vts_ifo->vtsi_mat->nr_of_vts_subp_streams;

	if(subtitles >= 0)
		return subtitles;
	else
		return 0;

}

uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track) {

	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	pgc_t *pgc = vts_pgcit->pgci_srp[vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn - 1].pgc;
	uint8_t i = 0;
	uint8_t idx = 0;

	for(idx = 0; idx < DVD_SUBTITLE_STREAM_LIMIT; idx++) {

		if(pgc->subp_control[idx] & 0x80000000)
			i++;

	}

	return i;

}

uint8_t dvd_subtitle_active(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track) {

	uint8_t i = dvd_track_active_subtitles(vmg_ifo, vts_ifo, title_track);

	if(subtitle_track > DVD_SUBTITLE_STREAM_LIMIT || subtitle_track > i)
		return 0;

	uint8_t ttn = dvd_track_ttn(vmg_ifo, title_track);
	pgcit_t *vts_pgcit = vts_ifo->vts_pgcit;
	uint16_t pgcn = vts_ifo->vts_ptt_srpt->title[ttn - 1].ptt[0].pgcn;
	pgc_t *pgc = vts_pgcit->pgci_srp[pgcn - 1].pgc;

	if(pgc->subp_control[subtitle_track - 1] & 0x80000000)
		return 1;

	return 0;

}

uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code) {

	uint8_t streams = dvd_track_subtitles(vts_ifo);

	if(streams == 0)
		return 0;

	uint8_t matches = 0;
	uint8_t i = 0;
	char str[DVD_SUBTITLE_LANG_CODE + 1] = {'\0'};

	for(i = 0; i < streams; i++) {

		strncpy(str, dvd_subtitle_lang_code(vts_ifo, i), DVD_SUBTITLE_LANG_CODE);

		if(strncmp(str, lang_code, DVD_SUBTITLE_LANG_CODE) == 0) {
			matches++;
		}

	}

	return matches;

}

bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code) {

	if(dvd_track_num_subtitle_lang_code_streams(vts_ifo, lang_code) > 0)
		return true;
	else
		return false;

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

// Note: Remember that the language code is set in the IFO
// See dvdread/ifo_print.c for same functionality (error checking)
const char *dvd_audio_lang_code(const ifo_handle_t *vts_ifo, const uint8_t audio_stream) {

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_stream];
	uint8_t lang_type = audio_attr->lang_type;

	if(lang_type != 1)
		return "";

	char lang_code[3] = {'\0'};

	snprintf(lang_code, DVD_AUDIO_LANG_CODE + 1, "%c%c", audio_attr->lang_code >> 8, audio_attr->lang_code & 0xff);

	return strndup(lang_code, DVD_AUDIO_LANG_CODE);

}

// FIXME see dvdread/ifo_print.c:196 for how it handles mpeg2ext with drc, no drc,
// same for lpcm; also a bug if it reports #5, and defaults to bug report if
// any above 6 (or under 0)
// FIXME check for multi channel extension
const char *dvd_audio_codec(const ifo_handle_t *vts_ifo, const uint8_t audio_stream) {

	const char *audio_codecs[7] = { "ac3", "", "mpeg1", "mpeg2", "lpcm", "sdds", "dts" };
	audio_attr_t *audio_attr =  &vts_ifo->vtsi_mat->vts_audio_attr[audio_stream];
	uint8_t audio_codec = audio_attr->audio_format;

	return strndup(audio_codecs[audio_codec], DVD_AUDIO_CODEC);

}

uint8_t dvd_audio_channels(const ifo_handle_t *vts_ifo, const uint8_t audio_stream) {

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_stream];
	uint8_t num_channels = audio_attr->channels + 1;

	if(num_channels >= 0)
		return num_channels;
	else
		return 0;

}

const char *dvd_audio_stream_id(const ifo_handle_t *vts_ifo, const uint8_t audio_stream) {

	audio_attr_t *audio_attr = &vts_ifo->vtsi_mat->vts_audio_attr[audio_stream];
	uint8_t audio_format = audio_attr->audio_format;
	uint8_t audio_id[7] = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};
	uint8_t audio_stream_id = audio_id[audio_format] + audio_stream;
	char str[DVD_AUDIO_STREAM_ID + 1] = {'\0'};

	snprintf(str, DVD_AUDIO_STREAM_ID + 1, "0x%x", audio_stream_id);

	return strndup(str, DVD_AUDIO_STREAM_ID);

}

// Have dvd_debug check for issues here.
// FIXME I want to have some kind of distinguishment in here, and for audio tracks
// if it's an invalid language.  If it's missing one, set it to unknown (for example)
// but if it's invalid, maybe guess that it's in English, or something?  Dunno.
// Having a best-guess approach might not be bad, maybe even look at region codes
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

const char *dvd_subtitle_stream_id(const uint8_t subtitle_track) {

	char str[DVD_SUBTITLE_STREAM_ID + 1] = {'\0'};

	snprintf(str, DVD_SUBTITLE_STREAM_ID + 1, "0x%x", 0x20 + subtitle_track);

	return strndup(str, DVD_SUBTITLE_STREAM_ID);

}
