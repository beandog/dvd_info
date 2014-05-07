#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/cdrom.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdread/ifo_read.h>
#include <dvdnav/dvdnav.h>
#include "dvdread/ifo_print.h"
#include "dvd_track.h"
#include "dvd_track_audio.h"
#include "dvd_track_subtitle.h"

/**
 * Functions used to get basic DVD data:
 * - Disc ID (libdvdread)
 * - Disc Title
 * - # tracks
 * - # VTS
 * - # Provider ID
 * - # Serial ID (libdvdnav)
 * - # VMG ID
 */

/**
 * Get the DVD title, which maxes out at a 32-character string.
 * All DVDs should have one.
 *
 * This is the same value as a volume name on an ISO, the string sits outside
 * of the encrypted content, so a simple seek to the location reveals the
 * contents.  In other words, this doesn't depend on libdvdread.  However, I
 * have had DVD drives that, for some reason or another, will complain if
 * the disc has not been decrypted first, so I would recommend decrypting the
 * CSS at some point before doing any reads from the data.  I could be
 * completely wrong about the CSS thing, but lots of anecdotal evidence says
 * otherwise.  In time, I hope to track down the real issue, but until then,
 * the generic suggestion stands. :)
 *
 * The title is going to be alpha-numeric, all upper-case, up to 32
 * characters in length.
 *
 * These should *not* be considered as unique identifiers for discs.
 *
 * Some examples:
 * - The Nightmare Before Christmas: NIGHTMARE
 * - Powerpuff Girls: POWERPUFF_GIRLS_DISC_1
 * - Star Trek: Voyager: VOYAGER_S7D1
 *
 * This whole function is mostly lifted from lsdvd.
 *
 * @param device_filename device filename
 * @param p char[33] to copy string to
 * @retval 0 success
 * @retval 1 could not open device
 * @retval 2 could not seek file
 * @retval 3 could not read the title
 */
int dvd_device_title(const char *device_filename, char *p);

/**
 * Get the number of tracks on a DVD
 *
 * The word 'title' and 'track' are sometimes used interchangeably when talking
 * about DVDs.  For context, I always use 'track' to mean the actual number of
 * distinct tracks on a DVD.
 *
 * I don't like to use the word 'title' when referencing a track, because one
 * video/audio track can have multiple "titles" in the sense of episodes.  Fex,
 * a Looney Tunes DVD track can have a dozen 7-minute shorts on it, and they
 * are all separated by chapter markers.
 *
 * At the very minimum, there is always going to be 1 track (seems obvious, but
 * I want to point that out).  At the very *most*, there will be 99 tracks.
 * Having 99 is unusual, and if you see it, it generally means that the DVD has
 * ARccOS copy-protection on it -- which, historically, has the distinct
 * feature of breaking libdvdread and libdvdnav sometimes.  99 tracks is rare.
 *
 * Movies that have no special features or extra content will have one track.
 *
 * Some examples of number of tracks on DVDs:
 *
 * Superman (Warner. Bros): 4
 * The Black Cauldron (Walt Disney): 99
 * Searching For Bobby Fischer: 4
 *
 * @param ifo dvdread IFO handle
 * @return num tracks
 */
uint16_t dvd_info_num_tracks(const ifo_handle_t *ifo);

/**
 * Get the number of VTS on a DVD
 *
 * @param ifo dvdread IFO handle
 * @return num VTS
 */
uint16_t dvd_info_num_vts(const ifo_handle_t *ifo);

/**
 * Get the provider ID
 *
 * The provider ID is a string that can be up to 32 characters long.  In my
 * experience, it looks like the content is arbitrarily set -- some DVDs are
 * mastered with the value being the studio, some with (what looks to be like)
 * the authoring software instead.  And then a lot are just numbers, or the
 * title of the movie.
 *
 * Unlike the DVD title, these strings have spaces and both upper and lower
 * case letters.
 *
 * Some examples:
 * - Challenge of the Super Friends: WARNER HOME VIDEO
 * - Felix the Cat (Disc 1): Giant Interactive
 * - The Escape Artist: ESCAPE_ARTIST
 * - Pink Panther (cartoons): LASERPACIFIC MEDIA CORPORATION
 *
 * It's not unusual for this one to be empty as well.
 *
 * These should *not* be considered as unique identifiers for discs.
 *
 * @param ifo dvdread IFO handle
 * @param p char[33] to copy string to
 */
void dvd_info_provider_id(const ifo_handle_t *ifo, char *p);

/**
 * Get the DVD's VMG id
 * It's entirely possible, and common, that the string is blank.  If it's not
 * blank, it is probably 'DVDVIDEO-VMG'.
 *
 * @param ifo libdvdread IFO handle
 * @param p char[13] to copy string to
 */
void dvd_info_vmg_id(const ifo_handle_t *ifo, char *p);

/**
 * Get the DVD side
 *
 * Some DVDs will have content burned on two sides -- for example there is
 * a widescreen version of the movie one one, and a fullscreen on the other.
 *
 * I haven't ever checked this before, so I don't know what DVDs are authored
 * with.  I'm going to simply return 2 if it's set to that, and a 1 otherwise.
 *
 * @param ifo libdvdread IFO handle
 * @return DVD side
 */
uint8_t dvd_info_side(const ifo_handle_t *ifo);

/**
 * Get the DVD serial id from dvdnav
 *
 * Serial ID gathered using libdvdnav.  I haven't been able to tell if the
 * values are unique or not, but I'd gather that they are not.  It's a 16
 * letter string at the max.
 *
 * Some examples:
 * - Good Night, and Good Luck: 33905bf9
 * - Shipwrecked! (PAL): 3f69708e___mvb__
 *
 * I would recommend *not* using these as unique identifiers -- mostly because
 * I don't know how they are generated.
 *
 * @param dvdnav dvdnav_t
 * @param p char[17] to copy the string to
 */
void dvd_info_serial_id(dvdnav_t *dvdnav, char *p);

/**
 * Find the longest track
 *
 * This will loop through all the tracks and see which one is longest.  If
 * there are two tracks of the same max length, set it to the *first* one.
 *
 * In that sense, this function should *not* be used with the intent to guess
 * that the longest track is also the feature track.  However, generally
 * speaking, DVDs are usually authored with the feature track coming in at
 * one of the first ones.
 *
 * A proper feature track function is going to examine each track and find
 * the one that is both the longest and has audio streams (as well as a few
 * other checks).  IOW, this function returns exactly what it's asked for:
 * the longest track number.
 *
 * @param dvdread_dvd libdvdread DVD handler
 * @return track number
 */
uint16_t dvd_info_longest_track(dvd_reader_t *dvdread_dvd);

/**
 * Find the longest track of all the tracks that *also* have subtitles.
 *
 * This can be confusing, so to be very specific here is how it works
 * 1. Examine ALL tracks
 * 2. If a track HAS SUBTITLES, then it is compared against the others
 * 3. Returns the LONGEST TRACK with SUBTITLES
 *
 * In other words:
 * LONGEST TRACK *may* or *may not* be the LONGEST TRACK WITH SUBTITLES
 *
 * This is a helper function.  It may or may not come in useful.  It will most
 * likely come in handy when trying to guess what the feature track is.  If it
 * comes down to multiple tracks with the same length, then this can possibly
 * flush out the fake ones.
 *
 * The chances that there are DVDs that are mastered so as to confuse the
 * software who guesses that the longest track is the feature one, but it is
 * not is possible (I wouldn't put it past studios to do that).  However, I
 * cannot think of any off the top of my head that do do this.  If I encounter
 * one, I'll document it here as an example, and also make a note of it at
 * http://dvds.beandog.org/problem_dvds as well.
 *
 * At the very least, it could serve as a good function to find the best track
 * when someone wants one that has subtitles to begin with.  Have fun!
 *
 * @param dvdread_dvd libdvdread handler
 * @return longest track number
 */
uint16_t dvd_info_longest_track_with_subtitles(dvd_reader_t *dvdread_dvd);

/**
 * Find the longest track that is also widescreen, with an aspect ratio of
 * 16x9.
 *
 * This is a helper function that can be used in narrowing down which track is
 * the feature track, and it comes down to aspect ratio being a deal-breaker.
 *
 * For an example, I'm using A Bug's Life on DVD.  It is mastered with two
 * copies of the movie -- fullscreen and widescreen -- on one side of the disc.
 * Track 1 has a length of 01:34:49.033 which is the fullscreen version, 4:3
 * aspect ratio.  However, track 6 has a length of 01:34:48.220, which is
 * 113 milliseconds less, but it is also the feature track, but widescreen, or
 * with a 16:9 aspect ratio.  In that example, if DVD playback software
 * guessed at the feature track as only being the one with the longest track
 * length, then it would be incorrect -- assuming the viewer prefers widescreen
 * over pan & scan (which is a generally valid assumption).
 *
 * For reference, dvd_id for A Bug's Life is: a8d71c6409a50a9b45432b2e0df7b425
 * Amazon: http://www.amazon.com/Bugs-Life-Two-Disc-Collectors/dp/B00007LVCM
 *
 * @param dvdread_dvd libdvdread handler
 * @return longest track number that has 16:9 aspect ratio or a -1 if no tracks
 * are found
 */
uint16_t dvd_info_longest_16x9_track(dvd_reader_t *dvdread_dvd);

/**
 * Find the longest track with a 4:3 aspect ratio
 *
 * Same reasoning for the earlier 16:9 function.
 *
 * @param dvdread_dvd libdvdread handler
 * @return longest track number that has 4:3 aspect ratio or a -1 if no tracks
 * are found
 */
uint16_t dvd_info_longest_4x3_track(dvd_reader_t *dvdread_dvd);

/**
 * Find the longest track marked as Letterbox
 *
 * The reason for this function can seem confusing considering there is an
 * earlier one that looks for a 16x9 aspect ratio.  The reason is this:
 * DVD tracks have two separate attributes that can be set:
 * - Aspect Ratio
 * - Display Format
 *
 * The possible aspect ratios are 4:3 and 16:9.
 * The possible display formats are Pan & Scan and Letterbox.
 *
 * It is *not* safe to assume that just because a track has a 4:3 aspect ratio
 * that it is also Pan & Scan.  There are DVDs that have feature titles where
 * black bars are added to letterbox video to force it to Pan & Scan.  These
 * are called non-anamorphic DVDs, and are generally a thorn in the side of
 * most DVD videophiles (and why we love Blu-ray so much).  That being said, it
 * is again not safe as a general rule to assume that the best feature track
 * also happens to be 16x9.
 *
 * It should be noted that it is very common for the Display Format attribute
 * to be *unset* on a DVD track, so, once again, this function should not be
 * considered as a definitive answer as to which track is the feature track.
 * It does, however, help narrow the options down if it comes to this level.
 *
 * @param dvdread_dvd libdvdread handler
 * @return longest track number that has a display format of Letterbox or a
 * -1 if no tracks with the attribute are found
 */
uint16_t dvd_info_longest_letterbox_track(dvd_reader_t *dvdread_dvd);

/**
 * Find the longest track marked as Pan & Scan
 *
 * See the previous commentary on the last function for reasoning.
 *
 * @param dvdread_dvd libdvdread handler
 * @return longest track number that has a display format of Pan & Scan, or a
 * -1 if no tracks with the attribute are found
 */
uint16_t dvd_info_longest_pan_scan_track(dvd_reader_t *dvdread_dvd);
