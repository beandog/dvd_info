#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dvdread/ifo_types.h>

/**
 * Get the lang code of a subtitle track for a title track
 *
 * @param vts_ifo dvdread track IFO handler
 * @param subtitle_track subtitle track number
 * @param p string pointer
 * @retval success
 * @retval 1 all the subpicture attributes are unset
 * @retval 2 the lang_code for the subpicture attribute is unset
 */
int dvd_track_subtitle_lang_code(const ifo_handle_t *vts_ifo, const int subtitle_track, char *p);

/**
 * Get the stream ID for a subtitle, an index that starts at 0x20
 *
 * @param vts_ifo dvdread track IFO handler
 * @param subtitle_track subtitle track number
 * @return stream ID
 */
int dvd_track_subtitle_stream_id(const ifo_handle_t *vts_ifo, const int subtitle_track);
