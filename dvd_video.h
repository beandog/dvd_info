#ifndef DVD_INFO_VIDEO_H
#define DVD_INFO_VIDEO_H

#include "dvd_track.h"

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

const char *dvd_track_video_format(const ifo_handle_t *vts_ifo);

const char *dvd_video_aspect_ratio(const ifo_handle_t *vts_ifo);

double dvd_track_fps(dvd_time_t *dvd_time);

const char *dvd_track_str_fps(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

#endif
