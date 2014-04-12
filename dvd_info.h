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
int dvd_info_num_tracks(ifo_handle_t *ifo) {

	int num_tracks;

	num_tracks = ifo->tt_srpt->nr_of_srpts;

	return num_tracks;

}

/**
 * Get the number of VTS on a DVD
 *
 * @param dvdread IFO handle
 * @return num VTS
 */
int dvd_info_num_vts(ifo_handle_t *ifo) {

	int num_vts;

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
 * @param dvdread IFO handle
 * @param char[33] to copy string to
 */
void dvd_info_provider_id(ifo_handle_t *ifo, char *tmpbuf) {

	int n = strlen(ifo->vmgi_mat->provider_identifier);

	// Copy the string only if it has characters, otherwise set the string
	// to zero length anyway.
	if(strlen > 0)
		strncpy(tmpbuf, ifo->vmgi_mat->provider_identifier, 32);
	else
		tmpbuf[0] = '\0';

}

/**
 * Get the DVD's VMG id
 * It's entirely possible, and common, that the string is blank.  If it's not
 * blank, it is probably 'DVDVIDEO-VMG'.
 *
 * @param libdvdread IFO handle
 * @param char[13] to copy string to
 */
void dvd_info_vmg_id(ifo_handle_t *ifo, char *tmpbuf) {

	int n = strlen(ifo->vmgi_mat->vmg_identifier);

	// Copy the string only if it has characters, otherwise set the string
	// to zero length anyway.
	if(strlen > 0)
		strncpy(tmpbuf, ifo->vmgi_mat->vmg_identifier, 12);
	else
		tmpbuf[0] = '\0';

}

/**
 * Get the DVD side
 *
 * Some DVDs will have content burned on two sides -- for example there is
 * a widescreen version of the movie one one, and a fullscreen on the other.
 *
 * I haven't ever checked this before, so I don't know what DVDs are authored
 * with.  I'm going to simply return 2 if it's set to that, and a 1 otherwise.
 */
int dvd_info_side(ifo_handle_t *ifo) {

	int side = ifo->vmgi_mat->disc_side;

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
 * @param dvdnav_t
 * @param
