#include "dvd_video.h"

uint8_t dvd_video_angles(const ifo_handle_t *vmg_ifo, const uint16_t track_number) {

	uint8_t angles = vmg_ifo->tt_srpt->title[track_number - 1].nr_of_angles;

	if(angles >= 0)
		return angles;
	else
		return 0;

}

/**
 * MPEG version
 *
 * 0 = MPEG1
 * 1 = MPEG2
 * 2 = Reserved, do not use
 * 3 = Reserved, do not use
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval 0 unknown
 * @retval 1 MPEG-1
 * @retval 2 MPEG-2
 */
uint8_t dvd_track_mpeg_version(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		return 1;
	else if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return 2;
	else
		return 0;

}

/**
 * Helper function to check if video codec is MPEG-1
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg1(const ifo_handle_t *vts_ifo) {

	if(dvd_track_mpeg_version(vts_ifo) == 1)
		return true;
	else
		return false;

}

/**
 * Helper function to check if video codec is MPEG-2
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg2(const ifo_handle_t *vts_ifo) {

	if(dvd_track_mpeg_version(vts_ifo) == 2)
		return true;
	else
		return false;

}

/**
 * Check if a video format is NTSC
 *
 * 0 = NTSC
 * 1 = PAL
 * 2 = Reserved, do not use
 * 3 = Reserved, do not use
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_ntsc_video(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.video_format == 0)
		return true;
	else
		return false;

}

/**
 * Check if a video format is PAL
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_pal_video(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.video_format == 1)
		return true;
	else
		return false;

}

/**
 * Get the video height
 *
 * @param vts_ifo dvdread track IFO handler
 * @return video height, or 0 for unknown
 */
uint16_t dvd_video_height(const ifo_handle_t *vts_ifo) {

	uint16_t video_height = 0;
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

	return video_height;

}

/**
 * Get the video width
 *
 * @param vts_ifo dvdread track IFO handler
 * @return video width, or 0 for unknown
 */
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

/**
 * Check for a valid aspect ratio (4:3, 16:9)
 *
 * @param vts_ifo dvdread track IFO handler
 * @return aspect ratio
 */
bool dvd_track_valid_aspect_ratio(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0 || vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return true;
	else
		return false;

}

/**
 * Check for 4:3 aspect ratio
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_aspect_ratio_4x3(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
		return true;
	else
		return false;

}

/**
 * Check for 16:9 aspect ratio
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_aspect_ratio_16x9(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return true;
	else
		return false;

}

/**
 * Get the permitted_df field for a track.  Returns the uint8_t
 * directly, error checking should be done to see if it's valid or not.
 *
 * 0 = Pan and Scan or Letterbox
 * 1 = Pan and Scan
 * 2 = Letterbox
 * 3 = Unset
 *
 * @param vts_ifo dvdread track IFO handler
 * @return uint8_t
 */
uint8_t dvd_video_df(const ifo_handle_t *vts_ifo) {

	uint8_t permitted_df = vts_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df >= 0)
		return permitted_df;
	else
		return 0;

}

/** FIXME
 *
 * All of these functions need to be correctly fixed:
 * - is letterbox (anamorphic widescreen or not)
 * - is widescreen (anamorphic widescreen or not)
 * - is pan & scan
 *
 * The problem is that permitted display format has *part* of the answer, while
 * the VTS itself can also have it's own "letterbox" tag.
 *
 * FIXME is that I need to clarify / document / code all the possible options
 * when it comes to widescreen (16x9), letterbox, pan & scan.
 *
 * See also http://stnsoft.com/DVD/ifo.html
 */

/**
 * Check for letterbox video
 *
 * This function looks at the permitted display format of the DVD.  Possible
 * values for this variable are:
 *
 * 0: Pan and Scan plus Letterbox
 * 1: Pan and Scan
 * 2: Letterbox
 * 3: Unset (most likely non-anormphic widescreen?)
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_video_letterbox(const ifo_handle_t *vts_ifo) {

	uint8_t permitted_df = vts_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df == 0 || permitted_df == 2)
		return true;
	else
		return false;

}

/**
 * Check for pan & scan video
 *
 * This function looks at the permitted display format of the DVD.  Possible
 * values for this variable are:
 *
 * 0: Pan & Scan plus Letterbox
 * 1: Pan & Scan
 * 2: Letterbox
 * 3: ???
 *
 * This function returns true if either the first condition or the second one
 * is true.  This means that a video may be pan & scan, or it may be non-
 * anomorphic widescreen.
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_video_pan_scan(const ifo_handle_t *vts_ifo) {

	uint8_t permitted_df = vts_ifo->vtsi_mat->vts_video_attr.permitted_df;

	if(permitted_df == 0 || permitted_df == 1)
		return true;
	else
		return false;

}

/*
 * Get the video codec for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval video codec
 */
const char *dvd_video_codec(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 0)
		return "MPEG1";
	else if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return "MPEG2";
	else
		return "";

}

/*
 * Get the video format for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval video format
 */
const char *dvd_track_video_format(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.video_format == 0)
		return "NTSC";
	else if(vts_ifo->vtsi_mat->vts_video_attr.mpeg_version == 1)
		return  "PAL";
	else
		return "";

}

/*
 * Get the video aspect ratio for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval aspect ratio
 */
const char *dvd_video_aspect_ratio(const ifo_handle_t *vts_ifo) {

	if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 0)
		return "4:3";
	else if(vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio == 3)
		return "16:9";
	else
		return "";

}

// FIXME needs documentation
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
