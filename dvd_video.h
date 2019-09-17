#ifndef DVD_INFO_VIDEO_H
#define DVD_INFO_VIDEO_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "dvd_track.h"
#include "dvd_vmg_ifo.h"

typedef struct {
	char codec[DVD_VIDEO_CODEC + 1];
	char format[DVD_VIDEO_FORMAT + 1];
	char aspect_ratio[DVD_VIDEO_ASPECT_RATIO + 1];
	uint16_t width;
	uint16_t height;
	bool letterbox;
	bool pan_and_scan;
	uint8_t df;
	char fps[DVD_VIDEO_FPS + 1];
	uint8_t angles;
} dvd_video_t;

uint8_t dvd_video_angles(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

uint8_t dvd_track_mpeg_version(const ifo_handle_t *vts_ifo);

bool dvd_track_mpeg1(const ifo_handle_t *vts_ifo);

bool dvd_track_mpeg2(const ifo_handle_t *vts_ifo);

bool dvd_track_ntsc_video(const ifo_handle_t *vts_ifo);

bool dvd_track_pal_video(const ifo_handle_t *vts_ifo);

uint16_t dvd_video_height(const ifo_handle_t *vts_ifo);

uint16_t dvd_video_width(const ifo_handle_t *vts_ifo);

bool dvd_track_valid_aspect_ratio(const ifo_handle_t *vts_ifo);

bool dvd_track_aspect_ratio_4x3(const ifo_handle_t *vts_ifo);

bool dvd_track_aspect_ratio_16x9(const ifo_handle_t *vts_ifo);

uint8_t dvd_video_df(const ifo_handle_t *vts_ifo);

bool dvd_video_letterbox(const ifo_handle_t *vts_ifo);

bool dvd_video_pan_scan(const ifo_handle_t *vts_ifo);

bool dvd_video_codec(char *dest_str, const ifo_handle_t *vts_ifo);

bool dvd_track_video_format(char *dest_str, const ifo_handle_t *vts_ifo);

bool dvd_video_aspect_ratio(char *dest_str, const ifo_handle_t *vts_ifo);

double dvd_track_fps(dvd_time_t *dvd_time);

bool dvd_track_str_fps(char *dest_str, const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

dvd_video_t *dvd_video_init(ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

#endif
