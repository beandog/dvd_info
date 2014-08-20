/**
 * Functions used to get information about a DVD track
 */

/**
 * Get the IFO number that a track resides in
 *
 * @param vmg_ifo dvdread handler for primary IFO
 * @return IFO number
 */
uint8_t dvd_track_ifo_number(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

uint8_t dvd_track_angles(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

/**
 * Get the track's VTS id
 * Possible that it's blank, usually set to DVDVIDEO-VTS otherwise.
 *
 * @param ifo libdvdread IFO handle
 */
char *dvd_track_vts_id(const ifo_handle_t *ifo);

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
uint16_t dvd_track_video_height(const ifo_handle_t *track_ifo);

/**
 * Get the video width
 *
 * @param track_ifo dvdread track IFO handler
 * @return video width, or 0 for unknown
 */
uint16_t dvd_track_video_width(const ifo_handle_t *track_ifo);

/**
 * Check for a valid aspect ratio (4:3, 16:9)
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
 * Get the permitted_df field for a track.  Returns the unsigned char
 * directly, error checking should be done to see if it's valid or not.
 *
 * 0 = Pan and Scan or Letterbox
 * 1 = Pan and Scan
 * 2 = Letterbox
 * 3 = Unset
 *
 * @param track_ifo dvdread track IFO handler
 * @return unsigned char
 */
unsigned char dvd_track_permitted_df(const ifo_handle_t *track_ifo);

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
 * 3: ???
 *
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_letterbox_video(const ifo_handle_t *track_ifo);

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
 * @param track_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_pan_scan_video(const ifo_handle_t *track_ifo);

/*
 * Get the video codec for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval video codec
 */
char *dvd_track_video_codec(const ifo_handle_t *track_ifo);

/*
 * Get the video format for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval video format
 */
char *dvd_track_video_format(const ifo_handle_t *track_ifo);

/*
 * Get the video aspect ratio for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval aspect ratio
 */
char *dvd_track_video_aspect_ratio(const ifo_handle_t *track_ifo);

double dvd_track_fps(dvd_time_t *dvd_time);

char *dvd_track_str_fps(dvd_time_t *dvd_time);

/**
 * Get the number of miliseconds for a track's playback time
 *
 * Same function as used in MPlayer and lsdvd
 *
 * @param dvd_time dvdread dvd_time struct
 * @return miliseconds
 */
unsigned int dvd_track_length(dvd_time_t *dvd_time);

unsigned int dvd_track_time_milliseconds(dvd_time_t *dvd_time);

unsigned int dvd_track_time_seconds(dvd_time_t *dvd_time);

unsigned int dvd_track_time_minutes(dvd_time_t *dvd_time);

unsigned int dvd_track_time_hours(dvd_time_t *dvd_time);

char *dvd_track_str_length(dvd_time_t *dvd_time);

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

/**
 * Get a string of the length of a chapter in a track.  Format hh:mm:ss.ms
 *
 * @param pgc track IFO program chain
 * @param chapter_number chapter number
 * @param p string pointer
 */
int dvd_track_str_chapter_length(const pgc_t *pgc, const uint8_t chapter_number, char *p);

/**
 * Get the audio language code for a track.  A two-character string that is a
 * short name for a language.
 *
 * Examples: en: English, fr: French, es: Spanish
 *
 * @param track_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @return language code
 */
char *dvd_track_audio_lang_code(const ifo_handle_t *track_ifo, const int audio_track);


/**
 * Get the codec for an audio track.
 *
 * Possible values: AC3, MPEG1, MPEG2, LPCM, SDDS, DTS
 * @param track_ifo dvdread track IFO handler
 * @param stream audio track number
 * @return audio codec
 */
char *dvd_track_audio_codec(const ifo_handle_t *track_ifo, const uint8_t stream);

/**
 * Get the number of channels for an audio track
 *
 * Human-friendly interpretation of what channels are:
 * 1: mono
 * 2: stereo
 * 3: 2.1, stereo and subwoofer
 * 4: front right, front left, rear right, rear left
 * 5: front right, front left, rear right, rear left, subwoofer
 *
 * @param track_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @return num channels
 */
int dvd_track_audio_num_channels(const ifo_handle_t *track_ifo, const int audio_track);

/**
 * Get the stream ID for an audio track
 *
 * Examples: 128, 129, 130
 *
 * @param track_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @return audio stream id
 */
char *dvd_track_audio_stream_id(const ifo_handle_t *track_ifo, const int audio_track);

/**
 * Get the lang code of a subtitle track for a title track
 *
 * @param vts_ifo dvdread track IFO handler
 * @param subtitle_track subtitle track number
 * @retval lang code
 */
char *dvd_track_subtitle_lang_code(const ifo_handle_t *vts_ifo, const int subtitle_track);

/**
 * Get the stream ID for a subtitle, an index that starts at 0x20
 *
 * This is only here for lsdvd output compatability.  The function just adds
 * the index to 0x20.
 *
 * @param subtitle_track subtitle track number
 * @return stream id
 */
char *dvd_track_subtitle_stream_id(const int subtitle_track);
