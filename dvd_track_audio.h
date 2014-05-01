#include <stdio.h>
#include <string.h>
#include <dvdread/ifo_types.h>

/**
 * Get the audio language code for a track.  A two-character string that is a
 * short name for a language.
 *
 * Examples: en: English, fr: French, es: Spanish
 *
 * @param track_ifo dvdread track IFO handler
 * @param audio_track audio track number
 * @param p string pointer
 * @return success
 */
int dvd_track_audio_lang_code(const ifo_handle_t *track_ifo, int audio_track, char *p);
