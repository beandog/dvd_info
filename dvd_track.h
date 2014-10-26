#ifndef _DVD_TRACK_H_
#define _DVD_TRACK_H_

/**
 * Functions used to get information about a DVD track
 */


/**
 * Check if the IFO is a VTS or not.
 *
 * libdvdread populates the ifo_handle with various data, but the structure is
 * the same for both a VMG IFO and a VTS one.  This does a few checks to make
 * sure that the ifo_handle passed in is a Video Title Set.
 *
 * @param ifo dvdread IFO handle
 * @return boolean
 */
bool ifo_is_vts(const ifo_handle_t *ifo);

/**
 * Get the IFO number that a track resides in
 *
 * @param vmg_ifo dvdread handler for primary IFO
 * @return IFO number
 */
uint16_t dvd_vts_ifo_number(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

uint8_t dvd_track_ttn(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

uint8_t dvd_track_angles(const ifo_handle_t *vmg_ifo, const uint16_t track_number);

/**
 * Get the track's VTS id
 * Possible that it's blank, usually set to DVDVIDEO-VTS otherwise.
 *
 * @param vts_ifo libdvdread IFO handle
 */
const char *dvd_vts_id(const ifo_handle_t *vts_ifo);

/**
 * Get the MPEG video codec version
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval 0 unknown
 * @retval 1 MPEG-1
 * @retval 2 MPEG-2
 */
uint8_t dvd_track_mpeg_version(const ifo_handle_t *vts_ifo);

/**
 * Helper function to check if video codec is MPEG-1
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg1(const ifo_handle_t *vts_ifo);

/**
 * Helper function to check if video codec is MPEG-2
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_mpeg2(const ifo_handle_t *vts_ifo);

/**
 * Check if a video format is NTSC
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_ntsc_video(const ifo_handle_t *vts_ifo);

/**
 * Check if a video format is PAL
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_pal_video(const ifo_handle_t *vts_ifo);

/**
 * Get the video height
 *
 * @param vts_ifo dvdread track IFO handler
 * @return video height, or 0 for unknown
 */
uint16_t dvd_track_video_height(const ifo_handle_t *vts_ifo);

/**
 * Get the video width
 *
 * @param vts_ifo dvdread track IFO handler
 * @return video width, or 0 for unknown
 */
uint16_t dvd_track_video_width(const ifo_handle_t *vts_ifo);

/**
 * Check for a valid aspect ratio (4:3, 16:9)
 *
 * @param vts_ifo dvdread track IFO handler
 * @return aspect ratio
 */
bool dvd_track_valid_aspect_ratio(const ifo_handle_t *vts_ifo);

/**
 * Check for 4:3 aspect ratio
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_aspect_ratio_4x3(const ifo_handle_t *vts_ifo);

/**
 * Check for 16:9 aspect ratio
 *
 * @param vts_ifo dvdread track IFO handler
 * @return boolean
 */
bool dvd_track_aspect_ratio_16x9(const ifo_handle_t *vts_ifo);

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
uint8_t dvd_track_permitted_df(const ifo_handle_t *vts_ifo);

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
bool dvd_track_letterbox_video(const ifo_handle_t *vts_ifo);

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
bool dvd_track_pan_scan_video(const ifo_handle_t *vts_ifo);

/*
 * Get the video codec for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval video codec
 */
const char *dvd_track_video_codec(const ifo_handle_t *vts_ifo);

/*
 * Get the video format for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval video format
 */
const char *dvd_track_video_format(const ifo_handle_t *vts_ifo);

/*
 * Get the video aspect ratio for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @retval aspect ratio
 */
const char *dvd_track_video_aspect_ratio(const ifo_handle_t *vts_ifo);

double dvd_track_fps(dvd_time_t *dvd_time);

const char *dvd_track_str_fps(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

const char *dvd_track_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

// Convert dvd time to total milliseconds
uint32_t dvdtime2msec(dvd_time_t *dvd_time);

uint32_t dvd_track_milliseconds(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

uint32_t dvd_time_milliseconds(dvd_time_t *dvd_time);

uint32_t dvd_time_seconds(dvd_time_t *dvd_time);

uint32_t dvd_time_minutes(dvd_time_t *dvd_time);

uint32_t dvd_time_hours(dvd_time_t *dvd_time);

const char *dvd_time_length(dvd_time_t *dvd_time);

/** Audio Streams **/

/**
 * Get the number of audio streams for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @return number of audio streams
 */
uint8_t dvd_track_num_audio_streams(const ifo_handle_t *vts_ifo);

/**
 * Examine the PGC for the track IFO directly and see if there are any audio
 * control entries marked as active.  This is an alternative way of checking
 * for the number of audio streams, compared to looking at the VTS directly.
 * This is useful for debugging, and flushing out either badly mastered DVDs or
 * getting a closer identifier of how many streams this has.
 *
 * Some software uses this number of audio streams in the pgc instead of the
 * one in the VTSI MAT, such as mplayer and HandBrake, which will skip over
 * the other ones completely.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @return number of PGC audio streams marked as active
 */
uint8_t dvd_track_num_active_audio_streams(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

/**
 * Look through the program chain to see if an audio track is flagged as
 * active or not.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @param audio_track audio track
 * @return boolean
 */
uint8_t dvd_track_active_audio_stream(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, const uint8_t audio_track);

/**
 * Get the number of audio streams for a specific language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return number of subtitles
 */
uint8_t dvd_track_num_audio_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

/**
 * Check if a DVD track has a specific audio language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return boolean
 */
bool dvd_track_has_audio_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

/** Subtitles **/

/**
 * Get the number of subtitle streams for a track
 *
 * @param vts_ifo dvdread track IFO handler
 * @return number of subtitles
 */
uint8_t dvd_track_subtitles(const ifo_handle_t *vts_ifo);

/**
 * Get the number of subtitle streams marked as active.
 *
 * When looking at which subtitles that can be accessed by software, such as
 * MPlayer or HandBrake, these are the amount that they will display.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @return number of active subtitles
 */
uint8_t dvd_track_active_subtitles(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track);

/**
 * Check if a subtitle stream is flagged as active or not.
 *
 * @param vmg_ifo dvdread track IFO handler
 * @param vts_ifo dvdread track IFO handler
 * @param title_track track number
 * @param subtitle_track track number
 * @return boolean
 */
uint8_t dvd_track_active_subtitle(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t title_track, uint8_t subtitle_track);

/**
 * Get the number of subtitle streams for a specific language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return number of subtitles
 */
uint8_t dvd_track_num_subtitle_lang_code_streams(const ifo_handle_t *vts_ifo, const char *lang_code);

/**
 * Check if a DVD track has a specific subtitle language
 *
 * @param vts_ifo dvdread track IFO handler
 * @param lang_code language code
 * @return boolean
 */
bool dvd_track_has_subtitle_lang_code(const ifo_handle_t *vts_ifo, const char *lang_code);

uint8_t dvd_track_chapters(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

uint8_t dvd_track_cells(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number);

/**
 * Get a string of the length of a chapter in a track.  Format hh:mm:ss.ms
 *
 */
const char *dvd_chapter_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t chapter_number);

/**
 * Get the audio language code for a track.  A two-character string that is a
 * short name for a language.
 *
 * DVD specification says that there is an ISO-639 character code here:
 * - http://stnsoft.com/DVD/ifo_vts.html
 * - http://www.loc.gov/standards/iso639-2/php/code_list.php
 * lsdvd uses 'und' (639-2) for undetermined if the lang_code and
 * lang_extension are both 0.
 *
 * Examples: en: English, fr: French, es: Spanish
 *
 * @param vts_ifo dvdread track IFO handler
 * @param audio_stream audio track number
 * @return language code
 */
const char *dvd_track_audio_lang_code(const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

uint32_t dvd_chapter_milliseconds(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

const char *chapter_ms_length(const uint32_t chapter_msecs);

uint8_t dvd_chapter_startcell(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, const uint8_t chapter_number);

uint32_t dvd_cell_milliseconds(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number);

const char *dvd_cell_length(const ifo_handle_t *vmg_ifo, const ifo_handle_t *vts_ifo, const uint16_t track_number, uint8_t cell_number);

/**
 * Get the codec for an audio track.
 *
 * Possible values: AC3, MPEG1, MPEG2, LPCM, SDDS, DTS
 * @param vts_ifo dvdread track IFO handler
 * @param audio_stream audio track number
 * @return audio codec
 */
const char *dvd_track_audio_codec(const ifo_handle_t *vts_ifo, const uint8_t audio_stream);

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
 * @param vts_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @return num channels
 */
uint8_t dvd_track_audio_num_channels(const ifo_handle_t *vts_ifo, const uint8_t audio_track);

/**
 * Get the stream ID for an audio track
 *
 * Examples: 128, 129, 130
 *
 * @param vts_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @return audio stream id
 */
const char *dvd_track_audio_stream_id(const ifo_handle_t *vts_ifo, const uint8_t audio_track);

/**
 * Get the lang code of a subtitle track for a title track
 *
 * @param vts_ifo dvdread track IFO handler
 * @param subtitle_track subtitle track number
 * @retval lang code
 */
const char *dvd_track_subtitle_lang_code(const ifo_handle_t *vts_ifo, const uint8_t subtitle_track);

/**
 * Get the stream ID for a subtitle, an index that starts at 0x20
 *
 * This is only here for lsdvd output compatability.  The function just adds
 * the index to 0x20.
 *
 * @param subtitle_track subtitle track number
 * @return stream id
 */
const char *dvd_track_subtitle_stream_id(const uint8_t subtitle_track);

// _DVD_TRACK_H_
#endif
