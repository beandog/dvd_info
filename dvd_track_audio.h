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
 * Examples: AC3 (Dolby Digital), DTS, SDDS, MPEG1, MPEG2, LPCM
 * @param track_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @param p string pointer
 * @return success
 */
int dvd_track_audio_codec(const ifo_handle_t *track_ifo, const int audio_track, char *p);

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
 * @return audio stream ID
 */
int dvd_track_audio_stream_id(const ifo_handle_t *track_ifo, const int audio_track);
