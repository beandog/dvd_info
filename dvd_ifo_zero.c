
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
 * @param device filename
 * @param char[33] to copy string to
 * @retval 0 success
 * @retval 1 could not open device
 * @retval 2 could not seek file
 * @retval 3 could not read the title
 */
int dvd_device_title(const char *device_filename, char *p) {

	char dvd_title[33] = {'\0'};
	FILE *filehandle = 0;
	size_t x;
	size_t y;

	// If we can't even open the device, exit quietly
	filehandle = fopen(device_filename, "r");
	if(filehandle == NULL) {
		return 1;
	}

	// The DVD title is actually on the disc, and doesn't need the dvdread
	// or dvdnav library to access it.  I should prefer to use them, though
	// to avoid situations where something freaks out for not decrypting
	// the CSS first ... so, I guess a FIXME is in order -- or use decss
	// first.
	if(fseek(filehandle, 32808, SEEK_SET) == -1) {
		fclose(filehandle);
		return 2;
	}

	x = fread(dvd_title, 1, 32, filehandle);
	dvd_title[32] = '\0';
	if(x == 0) {
		fclose(filehandle);
		return 3;
	}

	fclose(filehandle);

	// A nice way to trim the string. :)
	// FIXME: could use an rtrim function in general
	y = strlen(dvd_title);
	while(y-- > 2) {
		if(dvd_title[y] == ' ') {
			dvd_title[y] = '\0';
		}
	}

	strncpy(p, dvd_title, 32);

	return 0;

}

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
 * @param dvdread IFO handle
 * @return num tracks
 */
uint16_t dvd_info_num_tracks(const ifo_handle_t *ifo) {

	uint16_t num_tracks;

	num_tracks = ifo->tt_srpt->nr_of_srpts;

	return num_tracks;

}

/**
 * Get the number of VTS on a DVD
 *
 * @param dvdread IFO handle
 * @return num VTS
 */
uint16_t dvd_info_num_vts(const ifo_handle_t *ifo) {

	uint16_t num_vts;

	num_vts = ifo->vts_atrt->nr_of_vtss;

	return num_vts;
}

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
 * @param dvdread IFO handle
 * @param char[33] to copy string to
 */
void dvd_info_provider_id(const ifo_handle_t *ifo, char *p) {

	unsigned long n;

	n = strlen(ifo->vmgi_mat->provider_identifier);

	// Copy the string only if it has characters, otherwise set the string
	// to zero length anyway.
	if(n > 0)
		strncpy(p, ifo->vmgi_mat->provider_identifier, 32);
	else
		memset(p, '\0', 33);

}

/**
 * Get the DVD's VMG id
 * It's entirely possible, and common, that the string is blank.  If it's not
 * blank, it is probably 'DVDVIDEO-VMG'.
 *
 * @param libdvdread IFO handle
 * @param char[13] to copy string to
 */
void dvd_info_vmg_id(const ifo_handle_t *ifo, char *p) {

	unsigned long n;

	n = strlen(ifo->vmgi_mat->vmg_identifier);

	// Copy the string only if it has characters, otherwise set the string
	// to zero length anyway.
	if(n > 0)
		strncpy(p, ifo->vmgi_mat->vmg_identifier, 12);
	else
		memset(p, '\0', 13);

}

/**
 * Get the DVD side
 *
 * Some DVDs will have content burned on two sides -- for example there is
 * a widescreen version of the movie one one, and a fullscreen on the other.
 *
 * I haven't ever checked this before, so I don't know what DVDs are authored
 * with.  I'm going to simply return 2 if it's set to that, and a 1 otherwise.
 *
 * @param libdvdread IFO handle
 * @return DVD side
 */
uint8_t dvd_info_side(const ifo_handle_t *ifo) {

	uint8_t side = ifo->vmgi_mat->disc_side;

	if(side == 2)
		return 2;
	else
		return 1;

}

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
 * @param dvdnav_t
 * @param char[17] to copy the string to
 * @return success
 */
void dvd_info_serial_id(dvdnav_t *dvdnav, char *p) {

	const char *serial_id;
	unsigned long n;

	dvdnav_get_serial_string(dvdnav, &serial_id);

	n = strlen(serial_id);
	if(n > 0)
		strncpy(p, serial_id, 16);
	else
		memset(p, '\0', 17);

}
