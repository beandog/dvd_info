/**
 * Functions used to get information about a DVD track
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "dvdread/ifo_types.h"
#include "dvdread/ifo_read.h"
#include "dvd_track.h"

int dvd_track_video_codec(ifo_handle_t *track_ifo, char *video_codec);

uint8_t dvd_track_ifo_number(const ifo_handle_t *ifo_zero, const int track_number) {

	// TODO research
	// Should these be the same number
	// vts_ttn = ifo_zero->tt_srpt->title[title_track_idx].vts_ttn;

	int idx;
	uint8_t ifo_number;

	idx = track_number - 1;
	ifo_number = ifo_zero->tt_srpt->title[idx].title_set_nr;

	return ifo_number;

}

int dvd_track_mpeg_version(const ifo_handle_t *track_ifo) {

	unsigned char version = track_ifo->vtsi_mat->vts_video_attr.mpeg_version;

	if(version == 1)
		return 1;
	else if(version == 2)
		return 2;
	else
		return 0;

}

bool dvd_track_mpeg1(const ifo_handle_t *track_ifo) {

	unsigned char version = track_ifo->vtsi_mat->vts_video_attr.mpeg_version;

	if(version == 1)
		return true;
	else
		return false;

}

bool dvd_track_mpeg2(const ifo_handle_t *track_ifo) {

	unsigned char version = track_ifo->vtsi_mat->vts_video_attr.mpeg_version;

	if(version == 2)
		return true;
	else
		return false;

}

bool dvd_track_ntsc_video(const ifo_handle_t *track_ifo) {

	unsigned char format = track_ifo->vtsi_mat->vts_video_attr.video_format;

	if(format == 0)
		return true;
	else
		return false;

}

bool dvd_track_pal_video(const ifo_handle_t *track_ifo) {

	unsigned char format = track_ifo->vtsi_mat->vts_video_attr.video_format;

	if(format == 1)
		return true;
	else
		return false;

}

int dvd_track_video_height(const ifo_handle_t *track_ifo) {

	int video_height = 0;
	unsigned char picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;

	if(dvd_track_ntsc_video(track_ifo))
		video_height = 480;
	else if(dvd_track_pal_video(track_ifo))
		video_height = 576;

	if(picture_size == 3 && video_height > 0)
		video_height = video_height / 2;

	return video_height;

}

int dvd_track_video_width(const ifo_handle_t *track_ifo) {

	int video_width = 0;
	unsigned char picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;
	int video_height = dvd_track_video_height(track_ifo);

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

bool dvd_track_letterbox_video(const ifo_handle_t *track_ifo) {

	unsigned char letterbox = track_ifo->vtsi_mat->vts_video_attr.letterboxed;

	if(letterbox == 0)
		return false;
	else
		return true;

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


	int hours, minutes, seconds, milliseconds;

	hours = dvd_track_time_hours(dvd_time);
	minutes = dvd_track_time_minutes(dvd_time);
	seconds = dvd_track_time_seconds(dvd_time);
	milliseconds = dvd_track_time_milliseconds(dvd_time);

	snprintf(p, 13, "%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);

	p[14] = '\0';

}

/** Audio **/

// See dvd_track_audio.h for specific info to one audio stream

uint8_t dvd_track_num_audio_streams(const ifo_handle_t *track_ifo) {

	uint8_t num_audio_streams;

	num_audio_streams = track_ifo->vtsi_mat->nr_of_vts_audio_streams;

	return num_audio_streams;

}

/** Subtitles **/

uint8_t dvd_track_num_subtitles(const ifo_handle_t *track_ifo) {

	uint8_t num_subtitles;

	num_subtitles = track_ifo->vtsi_mat->nr_of_vts_subp_streams;

	return num_subtitles;

}
