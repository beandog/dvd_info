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
 * Helper function to check if video codec is MPEG-1
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
