#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include "dvd_info.h"
#include "dvd_track.h"

uint8_t dvd_track_ifo_number(const ifo_handle_t *vmg_ifo, const int track_number) {

	// TODO research
	// Should these be the same number
	// vts_ttn = vmg_ifo->tt_srpt->title[title_track_idx].vts_ttn;

	return vmg_ifo->tt_srpt->title[track_number - 1].title_set_nr;

}

uint8_t dvd_track_angles(const ifo_handle_t *vmg_ifo, const int track_number) {

	return vmg_ifo->tt_srpt->title[track_number - 1].nr_of_angles;

}

// FIXME check for invalid characters
char *dvd_track_vts_id(const ifo_handle_t *track_ifo) {

	for(unsigned long i = 0; i < strlen(track_ifo->vtsi_mat->vts_identifier); i++) {
		if(!isascii(track_ifo->vtsi_mat->vts_identifier[i]))
			return "";
	}

	return strndup(track_ifo->vtsi_mat->vts_identifier, DVD_TRACK_VTS_ID);

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
uint8_t dvd_track_mpeg_version(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		return 1;
	else if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return 2;
	else
		return 0;

}

bool dvd_track_mpeg1(const ifo_handle_t *track_ifo) {

	if(dvd_track_mpeg_version(track_ifo) == 1)
		return true;
	else
		return false;

}

bool dvd_track_mpeg2(const ifo_handle_t *track_ifo) {

	if(dvd_track_mpeg_version(track_ifo) == 2)
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

bool dvd_track_ntsc_video(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.video_format == 0)
		return true;
	else
		return false;

}

bool dvd_track_pal_video(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.video_format == 1)
		return true;
	else
		return false;

}

uint16_t dvd_track_video_height(const ifo_handle_t *track_ifo) {

	int video_height;
	unsigned char picture_size;

	video_height = 0;
	picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;

	if(dvd_track_ntsc_video(track_ifo))
		video_height = 480;
	else if(dvd_track_pal_video(track_ifo))
		video_height = 576;

	if(picture_size == 3 && video_height > 0) {
		video_height = video_height / 2;
		if(video_height < 0)
			video_height = 0;
	}

	return (uint16_t)video_height;

}

uint16_t dvd_track_video_width(const ifo_handle_t *track_ifo) {

	int video_width;
	uint16_t video_height;
	unsigned char picture_size;

	video_width = 0;
	video_height = dvd_track_video_height(track_ifo);
	picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;

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

	return (uint16_t)video_width;

}

bool dvd_track_valid_aspect_ratio(const ifo_handle_t *track_ifo) {

	unsigned char aspect_ratio;

	aspect_ratio = track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio;

	if(aspect_ratio == 0 || aspect_ratio == 3)
		return true;
	else
		return false;

}

bool dvd_track_aspect_ratio_4x3(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
		return true;
	else
		return false;

}

bool dvd_track_aspect_ratio_16x9(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return true;
	else
		return false;

}

unsigned char dvd_track_permitted_df(const ifo_handle_t *track_ifo) {

	return track_ifo->vtsi_mat->vts_video_attr.permitted_df;

}

/*
 * Display format
 * vtsi_mat->vts_video_attr.permitted_df
 *
 * 0 = Pan and Scan or Letterbox
 * 1 = Pan and Scan
 * 2 = Letterbox (anamorphic video, widescreen?)
 * 3 = Unset (most likely non-anormphic widescreen?)
 */
bool dvd_track_letterbox_video(const ifo_handle_t *track_ifo) {

	unsigned char permitted_df;

	permitted_df = track_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df == 0 || permitted_df == 2)
		return true;
	else
		return false;

}

bool dvd_track_pan_scan_video(const ifo_handle_t *track_ifo) {

	unsigned char permitted_df;

	permitted_df = track_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df == 0 || permitted_df == 1)
		return true;
	else
		return false;

}

char *dvd_track_video_codec(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		return "MPEG1";
	else if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return "MPEG2";
	else
		return "";

}

char *dvd_track_video_format(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.video_format == 0)
		return "NTSC";
	else if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return  "PAL";
	else
		return "";

}

char *dvd_track_video_aspect_ratio(const ifo_handle_t *track_ifo) {

	if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
		return "4:3";
	else if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return "16:9";
	else
		return "";

}

double dvd_track_fps(dvd_time_t *dvd_time) {

	double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};

	double fps = frames_per_s[(dvd_time->frame_u & 0xc0) >> 6];

	return fps;

}

char *dvd_track_str_fps(dvd_time_t *dvd_time) {

	char str[DVD_VIDEO_FPS + 1] = {'\0'};
	double fps;

	fps = dvd_track_fps(dvd_time);

	if(fps > 0) {

		snprintf(str, DVD_VIDEO_FPS + 1, "%02.02f", fps);

		return strndup(str, DVD_VIDEO_FPS);

	} else
		return "";

}

unsigned int dvd_track_length(dvd_time_t *dvd_time) {

	unsigned int framerates[4] = {0, 2500, 0, 2997};
	unsigned int framerate = framerates[(dvd_time->frame_u & 0xc0) >> 6];
	unsigned int milliseconds = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f)) * 3600000;
	milliseconds += (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f)) * 60000;
	milliseconds += (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f)) * 1000;

	if(framerate > 0)
		milliseconds += (((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000 / framerate;

	return milliseconds;

}

unsigned int dvd_track_time_milliseconds(dvd_time_t *dvd_time) {

	unsigned int framerates[4] = {0, 2500, 0, 2997};
	unsigned int framerate = framerates[(dvd_time->frame_u & 0xc0) >> 6];
	unsigned int milliseconds = 0;

	if(framerate > 0)
		milliseconds += (((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000 / framerate;

	return milliseconds;

}

unsigned int dvd_track_time_seconds(dvd_time_t *dvd_time) {

	unsigned int seconds = ((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f);

	if(seconds > 59)
		seconds -= 60;

	return seconds;

}

unsigned int dvd_track_time_minutes(dvd_time_t *dvd_time) {

	unsigned int minutes = ((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f);

	if(minutes > 59)
		minutes -= 60;

	return minutes;

}

unsigned int dvd_track_time_hours(dvd_time_t *dvd_time) {

	unsigned int hours = ((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f);

	if(hours > 59)
		hours -= 60;

	return hours;

}

char *dvd_track_str_length(dvd_time_t *dvd_time) {

	char length[DVD_TRACK_LENGTH + 1] = {'\0'};
	unsigned int hours;
	unsigned int minutes;
	unsigned int seconds;
	unsigned int milliseconds;

	hours = dvd_track_time_hours(dvd_time);
	minutes = dvd_track_time_minutes(dvd_time);
	seconds = dvd_track_time_seconds(dvd_time);
	milliseconds = dvd_track_time_milliseconds(dvd_time);

	snprintf(length, DVD_TRACK_LENGTH + 1, "%02u:%02u:%02u.%03u", hours, minutes, seconds, milliseconds);

	return strndup(length, DVD_TRACK_LENGTH);

}

/** Audio **/

uint8_t dvd_track_num_audio_streams(const ifo_handle_t *track_ifo) {

	return track_ifo->vtsi_mat->nr_of_vts_audio_streams;

}

// Used by dvd_debug
// Looks at the program control chain instead of the VTS for number of audio streams
// This should eliminate showing ghost audio streams that don't actually exist
// See https://github.com/thierer/lsdvd/commit/2adcc7d8ab1d3ccaa8b2aa294ad15ba5f72533d4
// See http://dvdnav.mplayerhq.hu/dvdinfo/pgc.html
// Looks for whether a stream available flag is set for each audio stream
uint8_t dvd_track_num_active_audio_streams(const ifo_handle_t *track_ifo) {

	uint8_t num_active_audio_streams;
	int idx;
	pgc_t *pgc;

	num_active_audio_streams = 0;
	pgc = track_ifo->vts_pgcit->pgci_srp[0].pgc;

	for(idx = 0; idx < 8; idx++) {

		if(pgc->audio_control[idx] & 0x8000)
			num_active_audio_streams++;

	}

	return num_active_audio_streams;

}

int dvd_track_num_audio_lang_code_streams(const ifo_handle_t *track_ifo, const char *p) {

	int num_track_audio_streams;
	int num_lang_streams;
	char lang_code[DVD_AUDIO_LANG_CODE + 1] = {'\0'};
	int audio_track;

	num_track_audio_streams = dvd_track_num_audio_streams(track_ifo);
	num_lang_streams = 0;

	for(audio_track = 0; audio_track < num_track_audio_streams; audio_track++) {

		strncpy(lang_code, dvd_track_audio_lang_code(track_ifo, audio_track), DVD_AUDIO_LANG_CODE);

		if(strncmp(lang_code, p, DVD_AUDIO_LANG_CODE) == 0) {
			num_lang_streams++;
		}

	}

	return num_lang_streams;

}

bool dvd_track_has_audio_lang_code(const ifo_handle_t *track_ifo, const char *lang_code) {

	if(dvd_track_num_audio_lang_code_streams(track_ifo, lang_code) > 0)
		return true;
	else
		return false;

}


/** Subtitles **/

uint8_t dvd_track_subtitles(const ifo_handle_t *track_ifo) {

	return track_ifo->vtsi_mat->nr_of_vts_subp_streams;

}

uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *track_ifo, const char *lang_code) {

	uint8_t streams;
	uint8_t matches;
	uint8_t idx;
	char str[DVD_SUBTITLE_LANG_CODE + 1] = {'\0'};

	streams = dvd_track_subtitles(track_ifo);
	matches = 0;

	for(idx = 0; idx < streams; idx++) {

		strncpy(str, dvd_track_subtitle_lang_code(track_ifo, idx), DVD_SUBTITLE_LANG_CODE);

		if(strncmp(str, lang_code, DVD_SUBTITLE_LANG_CODE) == 0) {
			matches++;
		}

	}

	return matches;

}

bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *track_ifo, const char *lang_code) {

	if(dvd_track_num_subtitle_lang_code_streams(track_ifo, lang_code) > 0)
		return true;
	else
		return false;

}

/**
 * Sourced from lsdvd.c.  I don't understand the logic behind it, and why the
 * original doesn't access pgc->cell_playback[cell_idx].playback_time directly.
 * Two things I do know: not to assume a cell is a chapter, and lsdvd's chapter
 * output has always worked for me, so I'm leaving it alone for now.
 *
 * This loops through *all* the chapters and gets the times, but only quits
 * once the specified one has been found.
 *
 * FIXME wrap my head around this some day.
 */
int dvd_track_str_chapter_length(const pgc_t *pgc, const uint8_t chapter_number, char *p) {

	uint8_t chapters;
	uint8_t chapter_idx;
	int program_map_idx;
	int cell_idx;
	char chapter_length[DVD_CHAPTER_LENGTH + 1] = {'\0'};

	chapters = pgc->nr_of_programs;
	chapter_idx = 0;
	cell_idx = 0;

	for(chapter_idx = 0; chapter_idx < pgc->nr_of_programs; chapter_idx++) {

		program_map_idx = pgc->program_map[chapter_idx + 1];

		if(chapter_idx == chapters - 1)
			program_map_idx = pgc->nr_of_cells + 1;

		while(cell_idx < program_map_idx - 1) {
			if(chapter_idx + 1 == chapter_number) {
				strncpy(chapter_length, dvd_track_str_length(&pgc->cell_playback[cell_idx].playback_time), DVD_TRACK_LENGTH);
				strncpy(p, chapter_length, DVD_TRACK_LENGTH);
				return 0;
			}
			cell_idx++;
		}

	}

	return 1;

}

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
char *dvd_track_audio_codec(const ifo_handle_t *track_ifo, const uint8_t stream) {

	unsigned char audio_codec;
	char *audio_codecs[7] = { "ac3", "", "mpeg1", "mpeg2", "lpcm", "sdds", "dts" };
	audio_attr_t *audio_attr;

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[stream];
	audio_codec = audio_attr->audio_format;

	return strndup(audio_codecs[audio_codec], DVD_AUDIO_CODEC);

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

char *dvd_track_audio_stream_id(const ifo_handle_t *track_ifo, const int audio_track) {

	int audio_id[7] = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};
	unsigned char audio_format;
	int audio_stream_id = 0;
	audio_attr_t *audio_attr;
	char str[DVD_AUDIO_STREAM_ID + 1] = {'\0'};

	audio_attr = &track_ifo->vtsi_mat->vts_audio_attr[audio_track];
	audio_format = audio_attr->audio_format;
	audio_stream_id = audio_id[audio_format] + audio_track;

	snprintf(str, DVD_AUDIO_STREAM_ID + 1, "0x%x", audio_stream_id);

	return strndup(str, DVD_AUDIO_STREAM_ID);

}

// TODO this function could stand a lot of strict error checking to see what the subtitle status is, and offer different return codes
// Have dvd_debug check for issues here.
// FIXME use isalpha()
// FIXME I want to have some kind of distinguishment in here, and for audio tracks
// if it's an invalid language.  If it's missing one, set it to unknown (for example)
// but if it's invalid, maybe guess that it's in English, or something?  Dunno.
// Having a best-guess approach might not be bad, maybe even look at region codes
char *dvd_track_subtitle_lang_code(const ifo_handle_t *track_ifo, const int subtitle_track) {

	char lang_code[3] = {'\0'};
	subp_attr_t *subp_attr;

	subp_attr = &track_ifo->vtsi_mat->vts_subp_attr[subtitle_track];

	// Same check as ifo_print
	if(subp_attr->type == 0 && subp_attr->lang_code == 0 && subp_attr->zero1 == 0 && subp_attr->zero2 == 0 && subp_attr->lang_extension == 0) {
		return "";
	}
	snprintf(lang_code, DVD_SUBTITLE_LANG_CODE + 1, "%c%c", subp_attr->lang_code >> 8, subp_attr->lang_code & 0xff);

	if(!isalpha(lang_code[0]) || !isalpha(lang_code[1]))
		return "";

	return strndup(lang_code, DVD_SUBTITLE_LANG_CODE);

}

char *dvd_track_subtitle_stream_id(const int subtitle_track) {

	char str[DVD_SUBTITLE_STREAM_ID + 1] = {'\0'};

	snprintf(str, DVD_SUBTITLE_STREAM_ID + 1, "0x%x", 0x20 + subtitle_track);

	return strndup(str, DVD_SUBTITLE_STREAM_ID);

}
