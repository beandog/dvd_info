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
 * Get the number of title tracks on a DVD
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
