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
int dvd_track_mpeg_version(ifo_handle_t *track_ifo);

/**
 * Helper function to check if video codec is MPEG-1
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg1(ifo_handle_t *track_ifo);

/**
 * Helper function to check if video codec is MPEG-2
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg2(ifo_handle_t *track_ifo);

/**
 * Check if a video format is NTSC
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_ntsc_video(ifo_handle_t *track_ifo);

/**
 * Check if a video format is PAL
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_pal_video(ifo_handle_t *track_ifo);

/**
 * Get the video height
 *
 * @param dvdread track IFO handler
 * @return video height, or 0 for unknown
 */
int dvd_track_video_height(ifo_handle_t *track_ifo);

/**
 * Get the video width
 *
 * @param dvdread track IFO handler
 * @return video width, or 0 for unknown
 */
int dvd_track_video_width(ifo_handle_t *track_ifo);

/**
 * Check for letterbox video
 *
 * @param dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_letterbox_video(ifo_handle_t *track_ifo);
