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

	int idx;
	unsigned char ifo_number;

	idx = track_number - 1;
	ifo_number = vmg_ifo->tt_srpt->title[idx].title_set_nr;

	return ifo_number;

}

unsigned char dvd_track_mpeg_version(const ifo_handle_t *track_ifo) {

	unsigned char mpeg_version;

	mpeg_version = track_ifo->vtsi_mat->vts_video_attr.mpeg_version;

	if(mpeg_version == 0)
		return 1;
	else if(mpeg_version == 1)
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

bool dvd_track_ntsc_video(const ifo_handle_t *track_ifo) {

	unsigned char video_format;

	video_format = track_ifo->vtsi_mat->vts_video_attr.video_format;

	if(video_format == 0)
		return true;
	else
		return false;

}

bool dvd_track_pal_video(const ifo_handle_t *track_ifo) {

	unsigned char video_format;

	video_format = track_ifo->vtsi_mat->vts_video_attr.video_format;

	if(video_format == 1)
		return true;
	else
		return false;

}

int dvd_track_video_height(const ifo_handle_t *track_ifo) {

	int video_height;
	unsigned char picture_size;

	video_height = 0;
	picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;

	if(dvd_track_ntsc_video(track_ifo))
		video_height = 480;
	else if(dvd_track_pal_video(track_ifo))
		video_height = 576;

	if(picture_size == 3 && video_height > 0)
		video_height = video_height / 2;

	return video_height;

}

int dvd_track_video_width(const ifo_handle_t *track_ifo) {

	int video_width;
	int video_height;
	unsigned char picture_size;

	video_width = 0;
	video_height = dvd_track_video_height(track_ifo);
	picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;

	if(video_height == 0)
		return 0;

	switch(picture_size) {

		case 0:
			video_width = 720;

		case 1:
			video_width = 704;

		case 2:
		case 3:
			video_width = 352;

		default:
			video_width = 0;

	}

	return video_width;

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

	unsigned char aspect_ratio;

	aspect_ratio = track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio;

	if(aspect_ratio == 0)
		return true;
	else
		return false;

}

bool dvd_track_aspect_ratio_16x9(const ifo_handle_t *track_ifo) {

	unsigned char aspect_ratio;

	aspect_ratio = track_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio;

	if(aspect_ratio == 3)
		return true;
	else
		return false;

}

bool dvd_track_letterbox_video(const ifo_handle_t *track_ifo) {

	unsigned char permitted_df;

	permitted_df = track_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df  == 0 || permitted_df == 2)
		return true;
	else
		return false;

}

bool dvd_track_pan_scan_video(const ifo_handle_t *track_ifo) {

	unsigned char permitted_df;

	permitted_df = track_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df  == 0 || permitted_df == 1)
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

void dvd_track_str_length(dvd_time_t *dvd_time, char *p) {

	int hours;
	int minutes;
	int seconds;
	int milliseconds;

	hours = dvd_track_time_hours(dvd_time);
	minutes = dvd_track_time_minutes(dvd_time);
	seconds = dvd_track_time_seconds(dvd_time);
	milliseconds = dvd_track_time_milliseconds(dvd_time);

	snprintf(p, 13, "%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);

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

	int num_audio_lang_code_streams;

	num_audio_lang_code_streams = dvd_track_num_audio_lang_code_streams(track_ifo, lang_code);

	if(num_audio_lang_code_streams > 0)
		return true;
	else
		return false;

}


/** Subtitles **/

uint8_t dvd_track_subtitles(const ifo_handle_t *track_ifo) {

	uint8_t streams;

	streams = track_ifo->vtsi_mat->nr_of_vts_subp_streams;

	return streams;

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

	uint8_t streams;

	streams = dvd_track_num_subtitle_lang_code_streams(track_ifo, lang_code);

	if(streams > 0)
		return true;

	return false;

}
