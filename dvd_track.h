/**
 * Functions used to get information about a DVD track
 */

/**
 * Get the IFO number that a track resides in
 *
 * @param vmg_ifo dvdread handler for primary IFO
 * @return IFO number
 */
uint8_t dvd_track_ifo_number(const ifo_handle_t *vmg_ifo, const int track_number);

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
 * FIXME this is probably unreliable to determine if a video is letterboxed
 * or not!
 *
 * There are two ways a DVD track could be construed as whether it's
 * letterbox or not:
 * - video attribute of IFO for letterbox is enabled
 * - video attribute of IFO for permitted DF is one of letterbox videos (0 or 2)
 *
 * THIS function only looks at the letterbox tag in the IFO, and NOT
 * at the permitted DF ones.  So, the FIXME is to either clarify what this
 * one is looking for, or add a function that determines wehter it is
 * letterbox or not based on both video attributes.
 *
 * For sake of proper referencing, it'd be good to examine DVDs and see
 * how they are mastered.
 *
 * Note that this boolean is *different than lsdvd output* for letterbox.
 * lsdvd will tag something as letterbox based on the permitted DF.
 *
 * Another thing to consider is, is the letterbox tag used at all?  I've
 * already seen DVDs that have letterboxed video (Star Wars: A New Hope)
 * but are not tagged as such.  So it could be that permitted DF should
 * be the deciding factor to begin with.  Again, more research is needed.
 *
 * UPDATE: I've changed the function to follow same syntax as lsdvd and
 * ifo_print.c as well.  If permitted_df is 0 (Pan & Scan plus Letterbox)
 * or 3 (Letterbox Only) then it is true.
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
 * Examine the PGC for the track IFO directly and see if there are any audio
 * control entries marked as active.  This is an alternative way of checking
 * for the number of audio streams, compared to looking at the VTS directly.
 * This is useful for debugging, and flushing out either badly mastered DVDs or
 * getting a closer identifier of how many streams this has.
 *
 * Haven't examined enough DVDs yet to verify that either one is more accurate.
 *
 * @param track_ifo dvdread track IFO handler
 * @return number of PGC audio streams marked as active
 */
uint8_t dvd_track_num_active_audio_streams(const ifo_handle_t *track_ifo);

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
