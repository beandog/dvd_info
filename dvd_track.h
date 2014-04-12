/**
 * Functions used to get information about a DVD track
 */

/**
 * Get the MPEG video codec version
 *
 * @param dvdread track IFO handler
 * @retval 0 unknown
 * @retval 1 MPEG-1
 * @retval 2 MPEG-2
 */
int dvd_track_mpeg_version(ifo_handle_t *track_ifo) {

	int version = track_ifo->vtsi_mat->vts_video_attr.mpeg_version;

	if(version == 1)
		return 1;
	else if(version == 2)
		return 2;
	else
		return 0;

}

/**
 * Helper function to check if video codec is MPEG-1
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg1(ifo_handle_t *track_ifo) {

	int version = track_ifo->vtsi_mat->vts_video_attr.mpeg_version;

	if(version == 1)
		return true;
	else
		return false;

}

/**
 * Helper function to check if video codec is MPEG-2
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg2(ifo_handle_t *track_ifo) {

	int version = track_ifo->vtsi_mat->vts_video_attr.mpeg_version;

	if(version == 2)
		return true;
	else
		return false;

}

/**
 * Check if a video format is NTSC
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_pal_video(ifo_handle_t *track_ifo) {

	int format = track_ifo->vtsi_mat->vts_video_attr.video_format;

	if(format == 0)
		return true;
	else
		return false;

}

/**
 * Check if a video format is PAL
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_pal_video(ifo_handle_t *track_ifo) {

	int format = track_ifo->vtsi_mat->vts_video_attr.video_format;

	if(format == 1)
		return true;
	else
		return false;

}

/**
 * Get the video height
 *
 * @param dvdread track IFO handler
 * @return video height, or 0 for unknown
 */
int dvd_track_video_height(ifo_handle_t *track_ifo) {

	int video_height = 0;
	int picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;

	if(dvd_track_ntsc_video(track_ifo))
		video_height = 480;
	else if(dvd_track_pal_video(track_ifo))
		video_height = 576;

	if(picture_size == 3 && video_height > 0)
		video_height = video_height / 2;

	return video_height;

}

/**
 * Get the video width
 *
 * @param dvdread track IFO handler
 * @return video width, or 0 for unknown
 */
int dvd_track_video_width(ifo_handle_t *track_ifo) {

	int video_width = 0;
	int picture_size = track_ifo->vtsi_mat->vts_video_attr.picture_size;
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

}

/**
 * Check for letterbox video
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_letterbox_video(ifo_handle_t *track_ifo) {

	int letterbox = track_ifo->vtsi_mat->vts_video_attr.letterboxed;

	if(format == 0)
		return false;
	else
		return true;

}
