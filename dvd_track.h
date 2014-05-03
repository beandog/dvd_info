/**
 * Functions used to get information about a DVD track
 */

/**
 * Get the IFO number that a track resides in
 *
 * @param ifo_zero dvdread handler for primary IFO
 * @return IFO number
 */
uint8_t dvd_track_ifo_number(const ifo_handle_t *ifo_zero, const int track_number);

/**
 * Get the MPEG video codec version
 *
 * @param track_ifo dvdread track IFO handler
 * @retval 0 unknown
 * @retval 1 MPEG-1
 * @retval 2 MPEG-2
 */
uint8_t dvd_track_mpeg_version(const ifo_handle_t *track_ifo);

/**
 * Helper function to check if video codec is MPEG-1
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg1(const ifo_handle_t *track_ifo);

/**
 * Helper function to check if video codec is MPEG-2
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg2(const ifo_handle_t *track_ifo);

/**
 * Check if a video format is NTSC
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_ntsc_video(const ifo_handle_t *track_ifo);

/**
 * Check if a video format is PAL
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_pal_video(const ifo_handle_t *track_ifo);

/**
 * Get the video height
 *
 * @param track_ifo dvdread track IFO handler
 * @return video height, or 0 for unknown
 */
int dvd_track_video_height(const ifo_handle_t *track_ifo);

/**
 * Get the video width
 *
 * @param track_ifo dvdread track IFO handler
 * @return video width, or 0 for unknown
 */
int dvd_track_video_width(const ifo_handle_t *track_ifo);

/**
 * Return the aspect ratio of a track
 *
 * This returns the value directly from the IFO attributes.  Error checking
 * should be done somewhere else.
 *
 * 0: 4:3
 * 3: 16:9
 *
 * @param track_ifo dvdread track IFO handler
 * @return aspect ratio
 */
bool dvd_track_valid_aspect_ratio(const ifo_handle_t *track_ifo);

/**
 * Check for 4:3 aspect ratio
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_aspect_ratio_4x3(const ifo_handle_t *track_ifo);

/**
 * Check for 16:9 aspect ratio
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_aspect_ratio_16x9(const ifo_handle_t *track_ifo);

/**
 * Check for letterbox video
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_letterbox_video(const ifo_handle_t *track_ifo);

/**
 * Get the number of miliseconds for a track's playback time
 *
 * Same function as used in MPlayer and lsdvd
 *
 * @param dvd_time dvdread dvd_time struct
 * @return miliseconds
 */
int dvd_track_length(dvd_time_t *dvd_time);

int dvd_track_time_milliseconds(dvd_time_t *dvd_time);

int dvd_track_time_seconds(dvd_time_t *dvd_time);

int dvd_track_time_minutes(dvd_time_t *dvd_time);

int dvd_track_time_hours(dvd_time_t *dvd_time);

void dvd_track_str_length(dvd_time_t *dvd_time, char *p);

/** Audio Streams **/

/**
 * Get the number of audio streams for a track
 *
 * @param track_ifo dvdread track IFO handler
 * @return number of audio streams
 */
uint8_t dvd_track_num_audio_streams(const ifo_handle_t *track_ifo);

/**
 * Get the number of audio streams for a specific language
 *
 * @param track_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return number of subtitles
 */
int dvd_track_num_audio_lang_code_streams(const ifo_handle_t *track_ifo, const char *lang_code);

/**
 * Check if a DVD track has a specific audio language
 *
 * @param track_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return boolean
 */
bool dvd_track_has_audio_lang_code(const ifo_handle_t *track_ifo, const char *lang_code);

/** Subtitles **/

/**
 * Get the number of subtitle streams for a track
 *
 * @param track_ifo dvdread track IFO handler
 * @return number of subtitles
 */
uint8_t dvd_track_subtitles(const ifo_handle_t *track_ifo);

/**
 * Get the number of subtitle streams for a specific language
 *
 * @param track_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return number of subtitles
 */
uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *track_ifo, const char *lang_code);

/**
 * Check if a DVD track has a specific subtitle language
 *
 * @param track_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return boolean
 */
bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *track_ifo, const char *lang_code);
