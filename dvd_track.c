#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include "dvd_track.h"
#include "dvd_track_audio.h"

unsigned char dvd_track_ifo_number(const ifo_handle_t *vmg_ifo, const int track_number) {

	// TODO research
	// Should these be the same number
	// vts_ttn = vmg_ifo->tt_srpt->title[title_track_idx].vts_ttn;

	return vmg_ifo->tt_srpt->title[track_number - 1].title_set_nr;

}

uint8_t dvd_track_angles(const ifo_handle_t *vmg_ifo, const int track_number) {

	return vmg_ifo->tt_srpt->title[track_number - 1].nr_of_angles;

}

void dvd_track_vts_id(const ifo_handle_t *ifo, char *p) {

	unsigned long n;

	n = strlen(ifo->vtsi_mat->vts_identifier);

	// Copy the string only if it has characters, otherwise set the string
	// to zero length anyway.
	if(n > 0)
		strncpy(p, ifo->vtsi_mat->vts_identifier, 12);
	else
		memset(p, '\0', 13);

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
unsigned char dvd_track_mpeg_version(const ifo_handle_t *track_ifo) {

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

int dvd_track_video_codec(ifo_handle_t *track_ifo, char *video_codec) {

	char *codec;

	if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		codec = "MPEG1";
	else if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		codec = "MPEG2";
	else
		codec = "";

	strncpy(video_codec, codec, 6);

	return 0;

}

int dvd_track_video_format(ifo_handle_t *track_ifo, char *video_format) {

	char *format;

	if(track_ifo->vtsi_mat->vts_video_attr.video_format == 0)
		format = "NTSC";
	else if(track_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		format = "PAL";
	else
		format = "";

	strncpy(video_format, format, 6);

	return 0;

}

int dvd_track_video_aspect_ratio(ifo_handle_t *track_ifo, char *video_aspect_ratio) {

	char *aspect_ratio;

	if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
		aspect_ratio = "4:3";
	else if(track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		aspect_ratio = "16:9";
	else {
		// Catch wrong integer value burned into DVD
		aspect_ratio = "";
	}

	strncpy(video_aspect_ratio, aspect_ratio, 5);

	return 0;

}

double dvd_track_fps(dvd_time_t *dvd_time) {

	double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};

	double fps = frames_per_s[(dvd_time->frame_u & 0xc0) >> 6];

	return fps;

}

int dvd_track_length(dvd_time_t *dvd_time) {

	int framerates[4] = {0, 2500, 0, 2997};
	int framerate = framerates[(dvd_time->frame_u & 0xc0) >> 6];
	int milliseconds = (((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f)) * 3600000;
	milliseconds += (((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f)) * 60000;
	milliseconds += (((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f)) * 1000;

	if(framerate > 0)
		milliseconds += (((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000 / framerate;

	return milliseconds;

}

int dvd_track_time_milliseconds(dvd_time_t *dvd_time) {

	int framerates[4] = {0, 2500, 0, 2997};
	int framerate = framerates[(dvd_time->frame_u & 0xc0) >> 6];
	int milliseconds = 0;

	if(framerate > 0)
		milliseconds += (((dvd_time->frame_u & 0x30) >> 3) * 5 + (dvd_time->frame_u & 0x0f)) * 100000 / framerate;

	return milliseconds;

}

int dvd_track_time_seconds(dvd_time_t *dvd_time) {

	int seconds = ((dvd_time->second & 0xf0) >> 3) * 5 + (dvd_time->second & 0x0f);

	if(seconds > 59)
		seconds -= 60;

	return seconds;

}

int dvd_track_time_minutes(dvd_time_t *dvd_time) {

	int minutes = ((dvd_time->minute & 0xf0) >> 3) * 5 + (dvd_time->minute & 0x0f);

	if(minutes > 59)
		minutes -= 60;

	return minutes;

}

int dvd_track_time_hours(dvd_time_t *dvd_time) {

	int hours = ((dvd_time->hour & 0xf0) >> 3) * 5 + (dvd_time->hour & 0x0f);

	if(hours > 59)
		hours -= 60;

	return hours;

}

char *dvd_track_str_length(dvd_time_t *dvd_time) {

	char length[13];
	int hours;
	int minutes;
	int seconds;
	int milliseconds;

	hours = dvd_track_time_hours(dvd_time);
	minutes = dvd_track_time_minutes(dvd_time);
	seconds = dvd_track_time_seconds(dvd_time);
	milliseconds = dvd_track_time_milliseconds(dvd_time);

	snprintf(length, 13, "%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);

	return strdup(length);

}

/** Audio **/

// See dvd_track_audio.h for specific info to one audio stream

uint8_t dvd_track_num_audio_streams(const ifo_handle_t *track_ifo) {

	uint8_t num_audio_streams;

	num_audio_streams = track_ifo->vtsi_mat->nr_of_vts_audio_streams;

	return num_audio_streams;

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
	char lang_code[3] = {'\0'};
	int audio_track;

	num_track_audio_streams = dvd_track_num_audio_streams(track_ifo);
	num_lang_streams = 0;

	for(audio_track = 0; audio_track < num_track_audio_streams; audio_track++) {

		dvd_track_audio_lang_code(track_ifo, audio_track, lang_code);

		if(strncmp(lang_code, p, 3) == 0) {
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
	char str[3] = {'\0'};

	streams = dvd_track_subtitles(track_ifo);
	matches = 0;

	for(idx = 0; idx < streams; idx++) {

		dvd_track_audio_lang_code(track_ifo, idx, str);

		if(strncmp(str, lang_code, 3) == 0) {
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
	char chapter_length[13] = {'\0'};

	chapters = pgc->nr_of_programs;
	chapter_idx = 0;
	cell_idx = 0;

	for(chapter_idx = 0; chapter_idx < pgc->nr_of_programs; chapter_idx++) {

		program_map_idx = pgc->program_map[chapter_idx + 1];

		if(chapter_idx == chapters - 1)
			program_map_idx = pgc->nr_of_cells + 1;

		while(cell_idx < program_map_idx - 1) {
			if(chapter_idx + 1 == chapter_number) {
				strncpy(chapter_length, dvd_track_str_length(&pgc->cell_playback[cell_idx].playback_time), 12);
				strncpy(p, chapter_length, 12);
				return 0;
			}
			cell_idx++;
		}

	}

	return 1;

}
