#ifndef DVD_INFO_VIDEO_H
#define DVD_INFO_VIDEO_H

#include "dvd_track.h"

uint8_t dvd_video_angles(ifo_handle_t *vmg_ifo, uint16_t track_number);

uint8_t dvd_track_mpeg_version(ifo_handle_t *vts_ifo);

bool dvd_track_mpeg1(ifo_handle_t *vts_ifo);

bool dvd_track_mpeg2(ifo_handle_t *vts_ifo);

bool dvd_track_ntsc_video(ifo_handle_t *vts_ifo);

bool dvd_track_pal_video(ifo_handle_t *vts_ifo);

uint16_t dvd_video_height(ifo_handle_t *vts_ifo);

uint16_t dvd_video_width(ifo_handle_t *vts_ifo);

bool dvd_track_valid_aspect_ratio(ifo_handle_t *vts_ifo);

bool dvd_track_aspect_ratio_4x3(ifo_handle_t *vts_ifo);

bool dvd_track_aspect_ratio_16x9(ifo_handle_t *vts_ifo);

uint8_t dvd_video_df(ifo_handle_t *vts_ifo);

bool dvd_video_letterbox(ifo_handle_t *vts_ifo);

bool dvd_video_pan_scan(ifo_handle_t *vts_ifo);

bool dvd_video_codec(char *dest_str, ifo_handle_t *vts_ifo);

bool dvd_track_video_format(char *dest_str, ifo_handle_t *vts_ifo);

bool dvd_video_aspect_ratio(char *dest_str, ifo_handle_t *vts_ifo);

double dvd_track_fps(dvd_time_t *dvd_time);

bool dvd_track_str_fps(char *dest_str, ifo_handle_t *vmg_ifo, ifo_handle_t *vts_ifo, uint16_t track_number);

#endif
